<?php

/**
 * @file
 *
 * Functions that add PTP data to database and/or report data about the
 * site to PTP or link the site to PTP service providers or otherwise
 * would not be interesting to people who are not PTP.
 *
 * See also:
 *  * lsd.drush.inc: Ensures basic functionality of Drupal/CiviCRM.
 *  * pbd.drush.inc: The PBD drush file configures a site to be a PowerBase
 *    with all required modules and extensions installed and all
 *    configuration options in place.
 */



/**
 * Implementation of hook_drush_command().
 */
function ptp_drush_command() {
  $items['ptp-update-site-stats'] =
    array(
      'description' => "Update information about the passed site in the PTP database.",
      'options' => array(
        'site' => 'the site to update (e.g. apen)',
        'contacts' => 'Number of contacts in the database',
        'logins' => 'Number of recent logins',
        'sibling' => 'Name of longshore sibling guest',
      ),
    );
  $items['ptp-configure'] =
    array('description' => "Ensure all PTP general settings are properly configured."
  );
  $items['ptp-check-sumfields'] =
    array('description' => "Report if any summary fields contribution triggers are missing."
  );
  $items['ptp-update-report'] =
    array('description' => "Print out report of modules and themes that need to be updated."
  );
  $items['ptp-fix-missing'] =
    array('description' => "Delete modules and themes that were installed but no longer are present."
  );
  return $items;
}

/**
 * Store statistics about a given site in the PTP database.
 *
 * By updating this information we can run reports from PTP's database
 * about each site's usage.
 **/
function drush_ptp_update_site_stats() {
  _civicrm_init();

  $sub = ptp_get_subdomain();
  if($sub != 'ptp') {
    drush_log("Please run against ptp.ourpowerbase.net only, not $sub.", 'error');
    return;
  }

  $site = drush_get_option('site', NULL);
  $contacts = drush_get_option('contacts', NULL);
  $logins = drush_get_option('logins', NULL);
  $sibling = drush_get_option('sibling', NULL);

  if(is_null($site)) {
    drush_log("Site is required.", 'error');
    return;
  }

  $site .= '.ourpowerbase.net';
  // Get the contact record for this site.

  // site_url sometimes has ending /, sometimes doesn't
  $site_url = "https://$site%";
  // custom_335 is the site URL
  $site_url_field = 'custom_335';
  // custom_472 is recent logins
  $logins_field = 'custom_472';
  // custom_471 is count of contacts
  $contacts_field = 'custom_471';
  // cusotm_470 is hosting frontend, aka sibling
  $sibling_field = 'custom_470';

  $params = array(
    $site_url_field => $site_url,
    'version' => 3,
    'return' => "${logins_field},${contacts_field},${sibling_field}"
  );
  $results = civicrm_api('Contact','get', $params);
  if($results['is_error'] != 0) {
    drush_log(dt("Error in api call to find contact id."), 'error');
    return;
  }
  if(count($results['values']) == 0) {
    drush_log(dt("Unable to find contact record for the site passed."), 'error');
    return;
  }
  $contact = array_pop($results['values']);
  $contact_id = $contact['contact_id'];
  $params = array(
    'id' => $contact_id,
    'version' => 3
  );

  if(!is_null($sibling)) {
    $existing_sibling = $contact[$sibling_field];
    if($existing_sibling != $sibling) {
      // Update the record.
      $params[$sibling_field] = $sibling;
    }
  }

  if(!is_null($contacts)) {
    $existing_contacts = $contact[$contacts_field];
    if($existing_contacts != $contacts) {
      // Update the record.
      $params[$contacts_field] = $contacts;
    }
  }
  if(!is_null($logins)) {
    $existing_logins = $contact[$logins_field];
    if($existing_logins != $logins) {
      // Update the record.
      $params[$logins_field] = $logins;
    }
  }
  $results = civicrm_api('Contact', 'update', $params);
  if($results['is_error'] != 0) {
    drush_log(dt("Unable to update the record."), 'error');
    return;
  }
  drush_log(dt("Record value has been updated."), 'notice');
}

/**
 * Implementation of command ptp-configure
 *
 * Run all functions that should execute when a site is configured. All
 * functions should be indempotent and relatively efficient when
 * run multiple times.
 */
function drush_ptp_configure() {
  drush_log(dt("Initializing CiviCRM"));
  _civicrm_init();

  // NOTE: configuring IMAP is handled separately since it needs to
  // manage IMAP passwords. See long_site_configure_imap bash function.

  // These functions are running everytime site-configure is run.
  ptp_insert_contacts();
  ptp_set_outbound_mail_settings();
  ptp_set_admin_password();
  ptp_create_help_menu();
}

/**
 * Insert PTP contacts into the database.
 *
 * We should maintain one individual contact in the database - with drupal
 * linked to Drupal uid 1 linked to an organizational record for PTP.
 **/
function ptp_insert_contacts() {
  try {
    $ind_contact_id = ptp_insert_ind_contact();
    $org_contact_id = ptp_insert_org_contact();
    if(!$ind_contact_id || !$org_contact_id) {
      return FALSE;
    }
    ptp_insert_relationship($ind_contact_id, $org_contact_id);
  }

  catch (CiviCRM_API3_Exception $e) {
    drush_log(dt("Failed to update PTP contacts in the database."), 'error');
    drush_log($e->getMessage(), 'error');
    return;
  }
}

/**
 * Insert PTP individual support contact as Drupal uid 1.
 */
function ptp_insert_ind_contact() {
  // Ensure that the drupal user with uid 1 is properly set.
  $target_super_user_email = 'support@progressivetech.org';
  $super_user_uid = 1;
  $super_user = user_load($super_user_uid);
  if($super_user->mail != $target_super_user_email) {
    // ensure no user already exists with the same email address
    $uid = db_select('users','u')
      ->fields('u')
      ->condition('mail', $target_super_user_email, '=')
      ->execute()
      ->fetchField();
    if(!empty($uid)) {
      $msg = dt(
        "Error: user with %email email address already exists (uid: %uid).",
        array('%email' => $target_super_user_email, '%uid' => $uid)
      );
      drush_log($msg, 'error');
      return;
    }
    $edit = array('mail' => $target_super_user_email);
    user_save($super_user,$edit);
  }

  // Now update the corresponding CiviCRM contact.

  // Ensure there *is* a corresponding CiviCRM contact. Normally it is
  // created the first time the user logs in. However, if we just installed
  // the site, it might not be there.  synchronizeUFMatch ensures both
  // the Contact is created and the UFMatch record is there.
  CRM_Core_BAO_UFMatch::synchronizeUFMatch($super_user, $super_user_uid,
    $target_super_user_email, 'Drupal');

  $params = array('uf_id' => 1 );
  $results = civicrm_api3('UFMatch', 'get', $params);
  $uf_match = array_pop($results['values']);
  if(!empty($uf_match['contact_id'])) {
    $contact_id = $uf_match['contact_id'];
    $params = array('id' => $contact_id);
    $first_name = 'Support';
    $last_name = 'Progressive Technology Project';
    ptp_update_contact($params, $first_name, $last_name, $target_super_user_email);
  }
  else {
    $msg = dt("Error: No CiviCRM contact for Drupal user with UID 1");
     drush_log($msg, 'error');
     return FALSE;
  }
  return $contact_id;
}

/**
 * Insert PTP organizational contact.
 */
function ptp_insert_org_contact() {
  // Ensure we have an organization record named PTP with the right
  // data.
  $params = array(
    'organization_name' => 'Progressive Technology Project',
    'contact_type' => 'Organization',
  );
  $results = civicrm_api3('Contact','get', $params);
  $org = array_pop($results['values']);

  // These are the values we want.
  $want_values = array(
    'email' => 'info@progressivetech.org',
    'phone' => '512-782-8478',
    'street_address' => 'PO Box 303190',
    'city' => 'Austin',
    'state_province_id' => '1042',
    'postal_code' => '78703'
  );

  // Prepare parameter arrays for updating and for creating.
  $create_params = $params;
  $update_params = $params;

  // If we have to create the record, these are the params to use.
  $create_params['api.email.create'] = array(
    'email' => $want_values['email'],
    'is_primary' => '1',
    'location_type_id' => '2', // 2 is work
  );
  $create_params['api.phone.create'] = array(
    'phone' => $want_values['phone'],
    'is_primary' => '1',
    'location_type_id' => '2', // 2 is work
  );
  $create_params['api.address.create'] = array(
    'street_address' => $want_values['street_address'],
    'city' => $want_values['city'],
    'state_province_id' => $want_values['state_province_id'],
    'postal_code' => $want_values['postal_code'],
    'location_type_id' => 2
  );

  if(count($org) != 0) {
    // We have a record already in the database. Ensure it has the right
    // values set or update if necessary.
    drush_log(dt("PTP Organization record exists."));
    $update = FALSE;
    while(list($k, $v) = each($want_values)) {
      if($org[$k] != $want_values[$k]) {
        // Get the appropriate parameter block.
        if($k == 'email') {
          $update_params['api.email.create'] =
            $create_params['api.email.create'];
        }
        elseif($k == 'phone') {
          $update_params['api.phone.create'] =
            $create_params['api.phone.create'];
        }
        else {
          $update_params['api.address.create'] =
            $create_params['api.address.create'];
        }
        $update = TRUE;
      }
    }
    if($update) {
      // We have to update the record.
      drush_log(dt("Updating PTP Organization contact."));
      $update_params['contact_id'] = $org['contact_id'];
      $result = civicrm_api3('Contact', 'create', $update_params);
    }
  }
  else {
    // We don't have a record in the database. Create from scratch.
    drush_log(dt("Creating PTP organization contact."));
    $results = civicrm_api3('Contact', 'create', $create_params);
    $org = array_pop($results['values']);
    drush_log(dt("Inserted PTP organization"));
  }
  if(empty($org['id'])) {
    drush_log(dt("Failed to obtain organization contact id."), 'error');
    return FALSE;
  }
  return $org['id'];
}

/**
 * Ensure relationship exists between support and org contact.
 */
function ptp_insert_relationship($ind_contact_id, $org_contact_id) {
  // Ensure there is a relationship. We use sql to properly search
  // both contact a and contact b sides.
  $sql = "SELECT id FROM civicrm_relationship WHERE (contact_id_a = %0
    AND contact_id_b = %1) OR (contact_id_a = %1 AND contact_id_b = %0)";
  $params = array(
    0 => array($org_contact_id, 'Integer'),
    1 => array($ind_contact_id, 'Integer'),
  );
  $dao = CRM_Core_DAO::executeQuery($sql, $params);
  $dao->fetch();
  if(empty($dao->id)) {
    drush_log(dt("Inserting PTP support/org contact relationship."));
    // We have to create one. The hard part is finding the right
    // relationship type since each site can modify this information.
    $sql = "SELECT id FROM civicrm_relationship_type WHERE name_a_b
      LIKE '%employe%' LIMIT 1";
    $dao = CRM_Core_DAO::executeQuery($sql);
    $dao->fetch();
    $relationship_type_id = $dao->id;
    $params = array(
      'contact_id_a' => $ind_contact_id,
      'contact_id_b' => $org_contact_id,
      'relationship_type_id' => $relationship_type_id
    );
    civicrm_api3('Relationship', 'create', $params);
    drush_log(dt("Created PTP contact relationship."));
  }
  else {
    drush_log(dt("PTP support/org contact relationship exists."));
  }
}

/**
 * Update the contact matching the params with the given values.
 *
 * This function is a helper for the ptp_insert_ind_contact() function.
 */
function ptp_update_contact($search_params, $first_name, $last_name, $email) {
  $code_ver = CRM_Utils_System::version();
  $sql = "SELECT version FROM civicrm_domain WHERE id = 1";
  $dao = CRM_Core_DAO::executeQuery($sql);
  $dao->fetch();
  $db_version = $dao->version;
  if($code_ver != $db_version) {
    // We are upgrading - so the code may expect fields that aren't yet present,
    // so skip this step.
    drush_log(dt("Not updating contact during upgrade."), 'info');
    return TRUE;
  }

  $results = civicrm_api3('Contact','get', $search_params);
  $contact = array_pop($results['values']);
  if(count($contact) == 0) {
    // This should never happen. We should always have a contact. We don't know
    // what the params are, but want to pass something helpful to
    // the error message, so take the value of the last param.
    $param_hint = array_pop($search_params);
    $msg = dt("No results found when updating civi contact, param hint: @param_hint.",
      array('@param_hint' => $param_hint));
    drush_log($msg, 'error');
    return FALSE;
  }
  // Only update if the values are different or empty.
  $fields = array('first_name', 'last_name', 'email');
  $update = FALSE;
  while(list(,$field) = each($fields)) {
    if(empty($contact[$field])) {
      $update = TRUE;
      break;
    }
    if($contact[$field] != $$field) {
      $update = TRUE;
      break;
    }
  }
  if($update) {
    $contact['id'] = $contact['contact_id'];
    $contact['first_name'] = $first_name;
    $contact['last_name'] = $last_name;
    $contact['email'] = $email;
    $contact['contact_type'] = 'Individual';

    $results = civicrm_api3('Contact', 'update', $contact);
    drush_log(dt("Powerbase user updated with first and last name."), 'notice');
    return array_pop($results['values']);
  }
  else {
    drush_log(dt("Powerbase user First/Last Names already updated."), 'notice');
    return $contact;
  }
}

/**
 * Set outbound email settings.
 */
function ptp_set_outbound_mail_settings() {
  // Set the outbound email settings
  drush_log(dt("Setting outbound mail settings."));
  $sub = ptp_get_subdomain();
  if(preg_match('/(^training|^demo|clone)$/', $sub)) {
    // For training, demo or clone sites, redirect to the database.
    // We don't want to send email on these sites ever.
    $new['outBound_option'] = 5;  // 5 is redirect to db
  } elseif ( getenv('LONG_LIVE') == 'y' ) {
    $new['outBound_option'] = 0;  // 0 is for smtp
    $new['smtpServer'] = 'bulk.ourpowerbase.net';
    $new['smtpPort'] = '25';
    $new['smtpAuth'] = 0;  // 0 is no auth
  } else {
    $new['outBound_option'] = 3;  // 3 is for mail()
  }

  $group_name = CRM_Core_BAO_Setting::MAILING_PREFERENCES_NAME;
  $mailingBackend = CRM_Core_BAO_Setting::getItem($group_name, 'mailing_backend' );
  $mailingBackend = array_merge($mailingBackend, $new);
  CRM_Core_BAO_Setting::setItem( $mailingBackend, $group_name, 'mailing_backend' );
}

/**
 * Get the name of the site.
 *
 * Should correspond to first part of domain name (e.g. caaav in
 * caaav.ourpowerbase.net)
 */
function ptp_get_subdomain() {
  $url_pieces = parse_url(CIVICRM_UF_BASEURL);
  $host_pieces = explode('.',$url_pieces['host']);
  return array_shift($host_pieces);
}

/**
 * Configure IMAP users in CiviCRM.
 *
 * Given a user and password, create the CiviCRM MailSettings that allow
 * CiviCRM to login to the returns and Activities email boxes.
 *
 */
function ptp_configure_imap_user($user, $pass, $host) {
  _civicrm_init();

  $mailSettings = new CRM_Core_DAO_MailSettings( );
  if(preg_match('/-returns$/',$user)) {
    $mailSettings->is_default = 1;
    $mailSettings->find( true );
    $mailSettings->name = "Returns";
  } elseif(preg_match('/-activities$/',$user)) {
    $mailSettings->is_default = 0;
    $mailSettings->find( true );
    $mailSettings->name = "Activities";
  }

  $mailSettings->domain_id = 1;
  $mailSettings->server = $host;
  $mailSettings->username = $user;
  $mailSettings->password = $pass;
  $mailSettings->domain = $host;

  // hard coded settings...
  $mailSettings->protocol = 1;  // 1 is imap
  $mailSettings->is_ssl = 1;
  $mailSettings->localpart = $user . '+';

  $mailSettings->save();
}

/**
 * Ensure the admin Drupal user's password is the same as db password.
 */
function ptp_set_admin_password() {
  require_once('includes/password.inc');
  // Get the currently set database password.
  $pass = $GLOBALS['databases']['default']['default']['password'];

  // Get the hashed password in the database.
  $sql = "SELECT pass FROM {users} WHERE uid = 1";
  $db_hashed_pass = db_query($sql)->fetch()->pass;

  if(empty($db_hashed_pass)) {
    // If we are initializing this password, we need a salt
    $db_hashed_pass = _password_generate_salt();
  }

  // Hash what the password should be.
  $hashed_pass = _password_crypt('sha512', $pass, $db_hashed_pass);

  // Compare...
  if($hashed_pass != $db_hashed_pass) {
    drush_log(dt("Updating Drupal admin password."));
    // Update
    db_update('users')
      ->fields(array(
        'pass' => $hashed_pass
      ))
      ->condition('uid', 1, '=')
      ->execute();
  }
  else {
    drush_log(dt("Drupal admin password already up-to-date."));
  }
}

/**
 * Setup special Help menu.
 *
 * PTP wants special items in the help menu to direct users to our help
 * resources. NOTE: CiviCRM changed the name of the menu from Help to
 * Support in 4.6.9.
 */
function ptp_create_help_menu() {
  // There are existing Civi menu items, and a bunch of random variations
  // that we've created in the past. To ensure we have a consistent help
  // menu, we first delete everything in the help menu, then rebuild it.

  // Depending on our version, we may look for "Help" or "Support"
  $menu_name = "Support";
  $code_ver = CRM_Utils_System::version();
  if(version_compare($code_ver, '4.6.9', '<')) {
    $menu_name = "Help";
  }
  drush_log(dt("Using menu name @menu_name", array('@menu_name' => $menu_name)));
  // Get list of menus
  $menus = CRM_Core_BAO_Navigation::getNavigationList();

  // Remove &nbsp; from the value names so we can more
  // reliably know if we've added them.
  array_walk($menus, 'ptp_strip_nbsp');

  // Ensure that the parent Help menu is there and get the id
  if(!in_array($menu_name,$menus)) {
    $params = array(
      'label' => $menu_name,
      'url' => '',
      'permission' => array(
        0 => 'access CiviCRM',
      ),
      'permission_operator' => 'AND',
       'is_active' => 1,
       'parent_id' => '',
    );
    // Rebuild array
    drush_log("Help/Support main menu item is not available. Adding it.", 'error');
    CRM_Core_BAO_Navigation::add($params);
    CRM_Core_BAO_Navigation::resetNavigation();
    $menus = CRM_Core_BAO_Navigation::getNavigationList();
    array_walk($menus, 'ptp_strip_nbsp');
  }

  // Get the id of the help menu
  $keys = array_keys($menus, $menu_name);
  $keys_count = count($keys);
  if($keys_count != 1) {
    // Hm... if they have more than one Help menu, then I'm not sure
    // what to do
    drush_log("Instead of 1, I found $keys_count support/help menu(s)... not trying to build PTP help menus.", 'error');
    print_r($keys);
    print_r($menus);
    return FALSE;
  }
  $help_id = array_pop($keys);

  // Delete all menus with this parent
  $domainID  = CRM_Core_Config::domainID( );
  $sql = "DELETE FROM civicrm_navigation WHERE parent_id = %1";
  $params = array(
    1 => array( $help_id, 'Integer'),
  );
  CRM_Core_DAO::executeQuery( $sql, $params );

  // Build out the submenus now.
  $submenus = array(
    'For Support, click this link, email support@progressivetech.org or call 512-782-8478' =>
      'https://ptp.ourpowerbase.net/help',
    'Upcoming Trainings' =>
      'https://ptp.ourpowerbase.net/civicrm/event/ical?reset=1&list=1&html=1',
    'CiviCRM - The Book' =>
      'http://book.civicrm.org/user/',
    'CiviCRM Community Forums' =>
      'https://civicrm.stackexchange.com/',
    'PowerBase FAQs' =>
      'http://network.progressivetech.org/tips',
    'Video Tutorials' =>
      'https://www.youtube.com/user/PowerOnPTP/playlists',
    'Change my password' =>
      'user',
    'About' =>
      'http://ourpowerbase.net/about/',
  );
  while(list($label, $url) = each($submenus)) {
    $params = array(
      'label' => $label,
      'url' => $url,
      'permission' => array(
        0 => 'access CiviCRM',
      ),
      'permission_operator' => 'AND',
       'is_active' => 1,
       'parent_id' => $help_id,
    );
    CRM_Core_BAO_Navigation::add($params);
  }
}

/**
 * Remove the string &nbsp; from the passed in value.
 *
 * Designed to be used in array_walk function.
 */
function ptp_strip_nbsp(&$value, $key) {
  $value = str_replace('&nbsp;', '', $value);
}

/**
 * Report if a site has sumfields installed and does not
 * have the contribution triggers installed.
 **/
function drush_ptp_check_sumfields() {
  _civicrm_init();
  if(function_exists('sumfields_get_update_trigger')) {
    if(!sumfields_get_update_trigger($table = 'civicrm_contribution')) {
      echo "\n\n+++++Problem!+++++\n\n";
    }
  }
}

/**
 * Command hook: fix missing modules and themes.
 */
function drush_ptp_fix_missing() {
  $sql = "SELECT name FROM system WHERE type = 'module'";
  $result = db_query($sql);
  while($name = $result->fetch()->name) {
    $filename = NULL;
    $show_error = FALSE;
    if(!drupal_get_filename('module', $name, $filename, $show_error)) {
      $sql = "DELETE FROM system WHERE name = :name";
      db_query($sql, array(':name' => $name));
    }
  }
}

/**
 * Command hook: report on modules/themes needing updates.
 */
function drush_ptp_update_report() {
  module_enable(array('update'));
  update_refresh();
  $updates_needed = array();
	if (!$available = update_get_available(TRUE)) {
		drush_log("No update data.", 'error');
	}
	else {
		module_load_include('inc', 'update', 'update.compare');
		$data = update_calculate_project_data($available);
		foreach($data as $project) {
      $recommended = NULL;
      if(array_key_exists('recommended', $project) ) {
        $recommended = $project['recommended'];
      }
      $versions = "(" .  $project['existing_version'] . " => " .
        $recommended . ")";
			$name = $project['name'];
			switch ($project['status']) {
				case UPDATE_UNKNOWN:
				case UPDATE_FETCH_PENDING:
				case UPDATE_NOT_FETCHED:
				case UPDATE_NOT_CHECKED:
					if($name != 'civicrm') {
						$updates_needed[$name] = "Didn't get update";
					}
					break;
				case UPDATE_NOT_SECURE:
					$updates_needed[$name] = "Security Update Needed ($versions)";
					break;
				case UPDATE_REVOKED:
				case UPDATE_NOT_SUPPORTED:
					break;
					$updates_needed[$name] = "No longer supported";
				case UPDATE_MAX_FETCH_ATTEMPTS:
					$updates_needed[$name] = "Too many update requests";
					break;
				case UPDATE_NOT_CURRENT:
					$updates_needed[$name] = "Update needed ($versions)";
				default:
					break;
			}
		}
  }
  module_disable(array('update'));
	print_r($updates_needed);
}
