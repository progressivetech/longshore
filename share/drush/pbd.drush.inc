<?php

/**
 * @file
 *
 * This file contains commands used to install and maintain PowerBase
 * databases. PowerBase is based on CiviCRM and is maintained by the
 * Progressive Technology Project (http://progressivetech.org/).
 *
 */

function pbd_drush_command() {
  $items = array();
  $items['pbd-configure'] =
    array('description' => "Ensure all PowerBase site are configured consisently.");
  $items['pbd-configure-once'] =
    array(
      'description' => "Configuration items that should only be run once on initial setup.",
      'options' => array(
        'force' => "Run even if we have already run the one time options."
      ),
    );
  $items['pbd-enable-debugging'] =
    array( 'description' => "Enable CiviCRM Debugging.");
  $items['pbd-disable-debugging'] =
    array( 'description' => "Disable CiviCRM Debugging.");
  $items['pbd-schema-version'] =
    array( 'description' => "Get CiviCRM schema version.");
  $items['pbd-code-version'] =
    array( 'description' => "Get CiviCRM code version");
  $items['pbd-obscure'] =
    array( 'description' => "Scrub all private data from the database. CAREFUL!!");
  $items['pbd-test-smart-groups'] =
    array( 'description' => "Rebuild all smart groups to find ones that are broken.");
  $items['pbd-update-assessments'] =
    array( 'description' => "Update all assessments (depends on net.ourpowerbase.assess extension).");
  $items['pbd-prepare-theme'] =
    array(
      'description' => "Setup a new theme to work as custom theme (remove blocks, configure theme, etc.)",
      'options' => array(
        'name' => 'The name of the theme to prepare.',
        'default' => 'Whether to set it as the default in themekey.'
      )
    );
  $items['pbd-subscription-links'] =
    array(
      'description' => "Output the cancel, update, etc links for a given contact.",
      'options' => array(
        'id' => 'The contact id of the person you want the links for.'
      )
    );
  $items['pbd-add-missing-indices'] =
    array(
      'description' => "Find and add any missing CiviCRM indices."
    );

  $items['pbd-setup-org-info'] =
    array(
      'description' => "Set default organization name and email address",
      'options' => array(
        'name' => 'The full name of the organization.',
        'email' => 'The primary email of the organization',
        'state' => 'The default state of the organization',
        'phone' => 'The default phone of the organization',
        'zip' => 'The zip of the organization',
        'address' => 'The address of the organization',
        'city' => 'The city of the organization',
      )
    );

  $items['pbd-restore-dedupe-rules'] = 
    array(
      'description' => 'Restore dedupe rules to defaults.'
    );
  $items['pbd-rebuild-multilingual-schema'] = 
    array(
      'description' => 'If site is multi-lingual, rebuilds the schema.'
    );
  return $items;
}

/*
 * ****************************************************
 *
 * All drush commands are defined below.
 *
 * ****************************************************
 */


/**
 * Rebuild the multi-lingual schema 
 *
 * Check to make sure the site is multi-lingual and if so, rebuild the schema.
 * This somehow doesn't get properly run when we upgrade.
 */

function drush_pbd_rebuild_multilingual_schema() {
  _civicrm_init();
  $sql = "SELECT locales FROM civicrm_domain WHERE locales IS NOT NULL";
  $dao = CRM_Core_DAO::executeQuery($sql);
  if ($dao->N == 0) {
    return TRUE;
  }
  $dao->fetch();
  $langs = explode(CRM_Core_DAO::VALUE_SEPARATOR, $dao->locales);
  CRM_Core_I18n_Schema::rebuildMultilingualSchema($langs);
  drush_log(dt("Updated schema with @langs", array('@langs' => print_r($langs, TRUE))), 'info');
}
/**
 * Restore dedupe rules.
 */
function drush_pbd_restore_dedupe_rules() {
  _civicrm_init();
  $sql_rule = "INSERT INTO civicrm_dedupe_rule VALUES (4,2,'civicrm_contact','organization_name',NULL,10),(5,2,'civicrm_email','email',NULL,10),(6,3,'civicrm_contact','household_name',NULL,10),(7,3,'civicrm_email','email',NULL,10),(9,5,'civicrm_contact','organization_name',NULL,10),(10,5,'civicrm_email','email',NULL,10),(11,6,'civicrm_contact','household_name',NULL,10),(12,6,'civicrm_email','email',NULL,10)";
  $sql_rule_group = "INSERT INTO civicrm_dedupe_rule_group VALUES (1,'Individual',20,'Supervised','IndividualSupervised','Name and Email (reserved)',1),(2,'Organization',10,'Supervised','OrganizationSupervised','Name and Email',0),(3,'Household',10,'Supervised','HouseholdSupervised','Name and Email',0),(4,'Individual',10,'Unsupervised','IndividualUnsupervised','Email (reserved)',1),(5,'Organization',10,'Unsupervised','OrganizationUnsupervised','Name and Email',0),(6,'Household',10,'Unsupervised','HouseholdUnsupervised','Name and Email',0),(7,'Individual',10,'General','IndividualGeneral','Name and Address (reserved)',1)";

  CRM_Core_DAO::executeQuery("DELETE FROM civicrm_dedupe_rule");
  CRM_Core_DAO::executeQuery("DELETE FROM civicrm_dedupe_rule_group");
  CRM_Core_DAO::executeQuery($sql_rule_group);
  CRM_Core_DAO::executeQuery($sql_rule);
}

/**
 * Update organization info
 */
function drush_pbd_setup_org_info() {
  $name = drush_get_option('name', null);
  $email = drush_get_option('email', null);
  $phone = drush_get_option('phone', null);
  $state = drush_get_option('state', null);
  $address = drush_get_option('address', null);
  $city = drush_get_option('city', null);
  $zip = drush_get_option('zip', null);

  // Let's make all fields required so we don't accidentally set things up
  // partially.
  $required = [ 'name', 'email', 'phone', 'state', 'address', 'city', 'zip' ];
  foreach($required as $req) {
    $val = $$req;
    if (empty($val)) {
      drush_log(dt("Can't set org info, @req is required.", array('@req' => $req)), 'error');
      return;
    }
  }
  _civicrm_init();
  // This should be passed as an abbreviation, so we first have to convert
  // it to the ID and if we fail we bail.
  try {
    $state_province_id = civicrm_api3('StateProvince', 'getvalue', [
      'abbreviation' => trim($state),
      'country_id' => 1228,
      'return' => 'id',
    ]);

  }
  catch (CiviCRM_API3_Exception $e) {
    drush_log(dt("Failed to get state province id for @state. Bailing!", array('@state' => $state)), 'error');
    drush_log($e->getMessage(), 'error');
    return;
  }

  // Set Drupal settings.
  variable_set('site_mail', $email);
  variable_set('site_name', "${name} Powerbase");

  // Now CiviCRM ones.
 
  // Make the state the default state. Amost all of our groups are local, so this makes sense.
  civicrm_api3('Setting', 'create', [ 'defaultContactStateProvince' => $state_province_id ]);

  // Make US the only available country in localization. 1228 is the id for the country US.
  civicrm_api3('Setting', 'create', [ 'countryLimit' => "1228" ]);

  // Set the domain.
  $params = array(
    'id' => CRM_Core_Config::domainID(),
  );
  $results = civicrm_api3('Domain', 'get', $params);
  // We can only update the name here.
  $values = array_pop($results['values']);
  $values['name'] = $name;
  civicrm_api3('Domain', 'update', $values);

  // Now we can update the contact manually.
  $contact_id = $values['contact_id'];
  $params = array(
    'id' => $contact_id,
    'sort_name' => $name,
    'display_name' => $name,
    'legal_name' => $name,
    'organization_name' => $name,
  );
  civicrm_api3('Contact', 'create', $params);

  // Only add email, phone and address if they don't already exist.
  // Test by trying to retrieve them.

  $email_params = array(
    'contact_id' => $contact_id,
    'location_type_id' => 2,
    'email' => $email,
    'is_primary' => 1,
  );
  try {
    civicrm_api3('Email', 'getsingle', $email_params);
  }
  catch (CiviCRM_API3_Exception $e) {
    if ($e->getMessage() == 'Expected one Email but found 0') {
      // It has not been added, so add it.
      civicrm_api3('Email', 'create', $email_params);
    }
    else {
      drush_log($e->getMessage(), 'error');
      return;
    }
  }

  $phone_params = array(
    'contact_id' => $contact_id,
    'location_type_id' => 2,
    'phone' => $phone,
    'is_primary' => 1,
  );
  try {
    civicrm_api3('Phone', 'getsingle', $phone_params);
  }
  catch (CiviCRM_API3_Exception $e) {
    if ($e->getMessage() == 'Expected one Phone but found 0') {
      // It has not been added, so add it.
      civicrm_api3('Phone', 'create', $phone_params);
    }
    else {
      drush_log($e->getMessage(), 'error');
      return;
    }
  }

  $address_params = array(
    'contact_id' => $contact_id,
    'location_type_id' => 2,
    'street_address' => $address,
    'state_province_id' => $state_province_id,
    'city' => $city,
    'postal_code' => $zip,
    'is_primary' => 1,
  );
  try {
    civicrm_api3('Address', 'getsingle', $address_params);
  }
  catch (CiviCRM_API3_Exception $e) {
    if ($e->getMessage() == 'Expected one Address but found 0') {
      // It has not been added, so add it.
      civicrm_api3('Address', 'create', $address_params);
    }
    else {
      drush_log($e->getMessage(), 'error');
      return;
    }
  }

  // More CiviCRM settings.
  $emailName = '"' . $name . '" <' . $email. '>';
  civicrm_api3('optionValue', 'create', array(
    'label' => $emailName,
    'option_group_id' => 'from_email_address',
    'is_default' => 1,
  ));

}

/**
 * Find any missing CiviCRM indices and add them.
 */
function drush_pbd_add_missing_indices() {
  _civicrm_init();

  // Ensure uf_group names are unique
  $sql = "SELECT id, name, count(*) c FROM civicrm_uf_group GROUP BY name HAVING c > 1;";
  $dao = CRM_Core_DAO::executeQuery($sql);
  while($dao->fetch()) {
    $sql = "UPDATE civicrm_uf_group SET name = %0 WHERE name = %1 AND id != %2";
    $params = array(
      0 => array($dao->name . '1', 'String'),
      1 => array($dao->name, 'String'),
      2 => array($dao->id, 'Integer')
    );
    CRM_Core_DAO::executeQuery($sql, $params);
  }
  $missing_indices = CRM_Core_BAO_SchemaHandler::getMissingIndices();
  if($missing_indices) {
    CRM_Core_BAO_SchemaHandler::createMissingIndices($missing_indices);
  }
  else {
    drush_log("No missing indices", 'info');
  }
}

/**
 * Ensure we run as admin user
 *
 * Some configuration steps check permissions and we don't want to fail
 * on those.
 *
 */
function pbd_run_as_admin() {
  // Ensure we operate with admin privs
  global $user;
  $user = user_load(1);
}

/**
 * Run code to ensure all PowerBase sites are consistently
 * configured. This function should be able to run repeatedly
 * without problems.
 **/
function drush_pbd_configure() {
  drush_log(dt("Running configure code"), 'notice');

  pbd_run_as_admin();
  // _civicrm_init no longer creates config singleton.
  // This is necessary in order to have a fully functional
  // database.
  _civicrm_init();
  $config = CRM_Core_Config::singleton();

  // Can be removed after it has run on all sites.
  module_disable(array('civicrm_engage'));
  // For some reason drupal_uninstall_modules is not found. ??.
  //drupal_uninstall_modules(array('civicrm_engage'));
  // Fix demographics custom group:
  $sql = "UPDATE civicrm_custom_group SET style = 'Inline' WHERE name = 'Demographics'";
  CRM_Core_DAO::executeQuery($sql);
  $params['keys'] = array(
    'net.ourpowerbase.spouse'
  );
  civicrm_api3('Extension', 'install', $params);

  $codeVer = CRM_Utils_System::version();
  if(version_compare($codeVer, '5.21', '>=')) {
    // After 5.21, everyone gets setup and recent items.
    drush_shell_cd_and_exec("/var/www/powerbase", "/usr/local/bin/cv --user=admin ext:enable net.ourpowerbase.setup");
    drush_shell_cd_and_exec("/var/www/powerbase", "/usr/local/bin/cv --user=admin ext:enable org.civicrm.recentmenu");
    module_enable(array('civicrmtheme'));
  }

  /* Drupal Settings */
  pbd_enable_modules();
  pbd_configure_themekey();
  pbd_configure_jquery_update();
  pbd_create_drupal_roles();
  pbd_grant_anonymous_and_authenticated_role_access();
  variable_set('preprocess_css',1);
  variable_set('preprocess_js',1);
  // Turning off page caching too...
  // https://app.liquidplanner.com/space/48716/projects/show/5899954P
  variable_set('cache',0);
  variable_set('block_cache',1);
  variable_set('page_compression',1);

  // Don't notify users when they are unblocked or activated
  // (you can choose to notify them when you create their account,
  // which is the only way to create an account anyway...).
  variable_set('user_mail_status_activated_notify', 0);

  // Handle themes for all versions.
  theme_enable(array('newsflash', 'iframe', 'seven', 'shoreditch_companion_d7_theme'));
  pbd_set_iframe_settings();
  pbd_set_newsflash_settings();
  pbd_set_blocks_for_theme('newsflash');
  pbd_set_blocks_for_theme('iframe');

  theme_enable(array('powerbase'));
  pbd_set_powerbase_theme_settings();
  pbd_set_blocks_for_theme('powerbase');

  pbd_add_include_exclude_search_menu();
  menu_rebuild();

  // workaround for https://drupal.org/node/1454686
  // and https://drupal.org/node/1153072
  pbd_grant_admin_all_perms();
  variable_set('site_default_country','US');
  pbd_set_admin_theme();
  pbd_disable_webform_promote_to_front();

  /* CiviCRM Settings */
  pbd_add_message_templates();
  pbd_activate_cron_jobs();
  pbd_default_geo_settings();
  pbd_turn_off_pdf_receipt();
  pbd_turn_off_version_check();
  pbd_turn_off_empowered_by_civicrm_footer();
  pbd_ensure_civicrm_news_removed_from_dashboard();
  pbd_add_partner_greeting_options();
  pbd_enable_user_framework_logging();
  pbd_set_max_file_size();
  pbd_configure_google_recaptcha();
  pbd_configure_google_geo_map();
  pbd_disable_security_update_alert();
  pbd_configure_additional_fonts();
  pbd_disable_external_extension_repository();
  pbd_set_smart_group_refresh_mode();

  pbd_disable_checks();

  // This should go last so we can reset settigs set above (like
  // preprocess variables).
  if ( getenv('LONG_LIVE') == 'n' ) {
    pbd_configure_dev_site();
  }

}

function pbd_disable_checks() {
  // The following should be removed once all databases have been rebuilt to include
  // the proper my.cnf settings.
  civicrm_api3('StatusPreference', 'create', ['name' => 'checkMysqlUtf8mb4', 'ignore_severity' => 3]);

  // The following should be removed when we upgrade to buster.
  civicrm_api3('StatusPreference', 'create', ['name' => 'checkPhpVersion', 'ignore_severity' => 3]);
}

/**
 * Code to run once to ensure all PowerBase sites are consistently
 * configured. This function should be run once. If may overwrite
 * customizations made by a group.
 **/
function drush_pbd_configure_once() {
  _civicrm_init();
  pbd_run_as_admin();

  $force = drush_get_option('force', null);
  // should only be run once because it contains settings
  // that a group may want to change and that we don't
  // want to override.
  drush_log(dt("performing one-time configuration."), 'notice');
  if(1 == variable_get('one-time-configuration-set')) {
    if(!is_null($force)) {
      drush_log(dt("forcing one-time configuration even though it has already been set."), 'notice');
    }
    else {
      drush_log(dt("one-time configuration has already been done, skipping."), 'notice');
      return TRUE;
    }
  }

  $one_time = TRUE;

  pbd_set_optin_defaults();

  # All new sites get the newsflash theme until we finalize the
  # new powerbase one..
  variable_set('theme_default','newsflash');

  // Ensure all the civicrm components we need are enabled
  pbd_enable_pb_components();

  pbd_grant_anonymous_and_authenticated_role_access($one_time);

  // _civicrm_init no longer creates config singleton.
  // This is necessary in order to have a fully functional
  // database.
  $config = CRM_Core_Config::singleton();

  // Only allow user creation by admin
  pbd_lock_down_user_registration();

  pbd_enable_one_time_extensions();
  

  variable_set('site_frontpage','civicrm');

  pbd_disable_create_new_household_menu();
  pbd_disable_modules();
  pbd_set_default_civicrm_option_lists();
  pbd_adjust_display_preferences();
  // This causes problems on migrated CiviCRM installs.
  // pbd_delete_all_tags();
  pbd_add_image_upload_menu();
  pbd_set_default_dateformat_full();
  pbd_configure_default_cicero_mappings();
  pbd_unset_default_header();
  pbd_make_drupal_user_profile_reserved();
  pbd_differentiate_sides_to_equal_relationships();
  pbd_disable_communication_style();
  pbd_disable_event_details();
  pbd_remove_country_from_address_formats();
  pbd_enable_waitlist_participant_status_types();
  pbd_add_first_tag();
  pbd_configure_ckeditor();
  pbd_configure_autologout();
  pbd_enable_remote_profile_submissions();
  pbd_set_state_province_limit();
  pbd_dedupe_rules();

  pbd_configure_wkhtmltopdf();
  variable_set('one-time-configuration-set', 1);
}
 
/**
 * Enable modules and extensions that should not reappear
 *
 * Groups may choose to disable them later.
 */
function pbd_enable_one_time_extensions() {
  $keys = array(
    'net.ourpowerbase.sumfields',
    'net.ourpowerbase.constituentfields',
    'net.ourpowerbase.participantfields',
    'net.ourpowerbase.profiles',
    'net.ourpowerbase.engage',
    'net.ourpowerbase.search.eventcount',
    'net.ourpowerbase.search.spouse',
    'net.ourpowerbase.setup',
    'com.aghstrategies.uscounties',
    'eu.tttp.noverwrite',
    'uk.co.vedaconsulting.mosaico',
    'de.systopia.osm',
    'org.civicrm.shoreditch',
    'org.civicrm.recentmenu',
  );

  foreach($keys as $key) {
    $key = escapeshellarg($key);
    // We call a separate process to avoid all the crazy cache problems we have
    // enabling these in the same session.
    drush_shell_cd_and_exec("/var/www/powerbase", "/usr/local/bin/cv --user=admin ext:enable $key");
  }

  // Now modules
  $modules = array('civicrmtheme');
  module_enable($modules);


}



function pbd_configure_dev_site() {
  drush_log(dt("Configuring dev site settings."));

  # Enable devel module.
  $modules = array('devel');
  module_enable($modules);

  // Allow anon users to see dsm calls.
  user_role_grant_permissions(1, array('access devel information'));

  // Allow admin role access to sql tab in reports.
  drush_civicrm_enable_debug();
  user_role_grant_permissions(3, array('view report sql'));

  // Make it easier to debug css and js issues.
  variable_set('preprocess_css', 0);
  variable_set('preprocess_js', 0);

  // Replace stripe live keys with PTP test keys.
  $sk = 'sk_test_TlGdeoi8e1EOPC3nvcJ4q5UZ';
  $pk = 'pk_test_k2hELLGpBLsOJr6jZ2z9RaYh';

  $params = array(
    0 => array($sk, 'String'),
    1 => array($pk, 'String'),
  );
  $sql = "UPDATE civicrm_payment_processor SET user_name = %0, password = %1 WHERE is_test = 0 AND payment_processor_type_id IN (SELECT id FROM civicrm_payment_processor_type WHERE name = 'Stripe')";

  //CRM_Core_DAO::executeQuery($sql, $params);
}

/**
 * Configure CiviCRM to NOT use wkhtmltopdf.
 *
 * Using DomPDF to generate PDF's (the default in CiviCRM) is painfully slow.
 * However, on Debian we can't seem to get a version of wkhtml that is secure
 * See: https://support.ourpowerbase.net/ticket/2528
 **/
function pbd_configure_wkhtmltopdf() {
  _civicrm_init();
  pbd_setting('wkhtmltopdfPath', '/usr/bin/wkhtmltopdf');
}

/**
 * Set max file upload site.
 *
 * Ensure our max file size (for uploads) stays at a reasonable size
 */
function pbd_set_max_file_size() {
  // The number is in MB. Our php.ini file should allow up to 30MB.
  pbd_setting("maxFileSize", 20);
}

/**
 * Send CiviCRM errors to watchdog so users can access them. This is
 * particularly important for SMS errors.
 */
function pbd_enable_user_framework_logging() {
  _civicrm_init();
  pbd_setting('userFrameworkLogging', 1);
}

/**
 * Add contact_and_partner_first_name option to all greetings.
 */
function pbd_add_partner_greeting_options() {
  // the key is the option group name and the value is the option group value
  $new_greetings = array(
    'email_greeting' => 'Dear {partner.contact_and_partner_first_name}',
    'postal_greeting' => 'Dear {partner.contact_and_partner_first_name}',
    'addressee' => '{partner.contact_and_partner_full_name}'
  );

  // Find the option groups that *don't* have this value
  $sql = "SELECT g.id FROM civicrm_option_group g LEFT JOIN civicrm_option_value v " .
    "ON g.id = v.option_group_id AND v.name LIKE '%{partner.contact_and_partner_%' ".
    "WHERE g.name = %0 AND v.id IS NULL";
  while(list($option_group) = each($new_greetings)) {
    drush_log(dt("Checking option group @option_group.", array('@option_group' => $option_group)));
    $params = array(0 => array($option_group, 'String'));
    $dao = CRM_Core_DAO::executeQuery($sql, $params);
    $dao->fetch();
    if($dao->N == 1) {
      $msg =  dt("Option group @option_group is missing partner greeting value.",
          array('@option_group' => $option_group));
      drush_log($msg);
      // We have a match - we have to insert this value for this option group
      // filter = 1 means limit to individual contats.
      $params = array(
        'option_group_id' => $dao->id,
        'name' => $new_greetings[$option_group],
        'label' => $new_greetings[$option_group],
        'filter' => 1,
      );
      try {
        civicrm_api3('OptionValue', 'create', $params);
      }
      catch (CiviCRM_API3_Exception $e) {
        drush_log(dt("Failed to create greeting option."), 'error');
        drush_log($e->getMessage(), 'error');
        return;
      }
    }
  }
}

/**
 * FIXME: I think both of these commands are now
 * in civicrm.drush.inc
 **/
function drush_pbd_enable_debugging() {
  pbd_change_debugging(1);
}

function drush_pbd_disable_debugging() {
  pbd_change_debugging(0);
}

function drush_pbd_schema_version() {
  _civicrm_init();
  drush_print(CRM_Core_BAO_Domain::version());
}

function drush_pbd_code_version() {
  _civicrm_init();
  drush_print(CRM_Utils_System::version());
}

/**
 *
 * Used to obscure data so it can be sent to CiviCRM
 * core team members to fix bugs. This was taken from
 * the encryptDB.php script.
 *
 * FIXME: This should be re-posted upstream to civicrm.drush.inc.
 *
 **/
function drush_pbd_obscure() {
  $prompt = "Are you sure you want to proceed? This will destroy your database!";
  $default = 'n';
  $required = TRUE;
  $response = drush_prompt($prompt, $default, $required);
  if($response != 'y') {
    drush_print("Not running.");
    return;
  }
  drush_print("Obscuring database values...");
  define('CRM_ENCRYPT', 1);
  define('CRM_SETNULL', 2);
  $tables = array(
    'civicrm_contact' => array(
      'first_name' => CRM_ENCRYPT,
      'last_name' => CRM_ENCRYPT,
      'organization_name' => CRM_ENCRYPT,
      'household_name' => CRM_ENCRYPT,
      'sort_name' => CRM_ENCRYPT,
      'display_name' => CRM_ENCRYPT,
      'legal_name' => CRM_ENCRYPT,
    ),
    'civicrm_address' => array(
      'street_address' => CRM_ENCRYPT,
      'supplemental_address_1' => CRM_ENCRYPT,
      'supplemental_address_2' => CRM_ENCRYPT,
      'city' => CRM_ENCRYPT,
      'postal_code' => CRM_SETNULL,
      'postal_code_suffix' => CRM_SETNULL,
      'geo_code_1' => CRM_SETNULL,
      'geo_code_2' => CRM_SETNULL,
    ),
    'civicrm_website' => array(
      'url' => CRM_ENCRYPT,
    ),
    'civicrm_email' => array(
      'email' => CRM_ENCRYPT,
    ),
    'civicrm_phone' => array(
      'phone' => CRM_ENCRYPT,
    ),
  );

  foreach ($tables as $tableName => $fields) {
    $clauses = array();
    foreach ($fields as $fieldName => $action) {
      if ($action == CRM_ENCRYPT) {
        $clauses[] = "$fieldName = md5($fieldName)";
      }
      elseif ($action == CRM_SETNULL) {
        $clauses[] = "$fieldName = null";
      }
    }

    if (!empty($clauses)) {
      $clause = implode(',', $clauses);
      $query = "UPDATE $tableName SET $clause";
      CRM_Core_DAO::executeQuery($query);
    }
  }
}


function drush_pbd_test_smart_groups() {
  _civicrm_init();
  $group_ids = array();

  // See if group_id is being passed in.
  $given_group_id = drush_get_option('group_id', NULL);
  $after_group_id = explode(',', drush_get_option('after_group_id', NULL));
  if(!empty($given_group_id)) {
    $group_ids[$given_group_id] = NULL;
  }
  else {
    // Fetch all active groups that are smart groups
    $sql = 'SELECT id, title FROM civicrm_group WHERE saved_search_id IS NOT NULL AND is_active = 1 ORDER BY id WHERE id > %0';
    $dao = CRM_Core_DAO::executeQuery($sql, [ 0 => [ $after_group_id, 'Integer' ] ]);
    $total_duration = 0;
    while($dao->fetch()) {
      $group_ids[$dao->id] = $dao->title;
    }
  }
  while(list($id, $title) = each($group_ids)) {
    $start = time();
    if(empty($title)) {
      $msg = dt("Attempting to load Group @id.", array('@id' => $id));
    }
    else {
      $msg = dt("Attempting to load Group @id (@title).",
        array('@id' => $id, '@title' => $title), 'ok');
    }
    drush_log($msg, 'ok');
    // Load group
    $group = new CRM_Contact_DAO_Group();
    $group->id = $id;
    $group->find(TRUE);
    $force = TRUE;
    // Check to see if lock exists
    $lock = Civi\Core\Container::singleton()->get('lockManager')->acquire("data.core.group.{$id}");
    if (!$lock->isAcquired()) {
      drush_log(dt("Lock exists, not rebuilding."), 'error');
      continue;
    }
    CRM_Contact_BAO_GroupContactCache::load($group, $force);
    $end = time();
    $duration = $end - $start;
    $total_duration += $duration;
    $duration_formatted = number_format($duration/60, 2);
    $params = array('@id' => $id, '@duration' => $duration_formatted);
    // Now get count of records returned
    $sql = "SELECT COUNT(*) AS count FROM civicrm_group_contact_cache WHERE group_id = %0";
    $sql_params = array(0 => array($id, 'Integer'));
    try {
      $dao = CRM_Core_DAO::executeQuery($sql, $sql_params);
      $dao->fetch();
      $params['@count'] = $dao->count;
      drush_log(dt("Completed loading Group @id in @duration minutes with @count contacts.", $params), 'ok');
    }
    catch (Exception $e) {
      drush_log(dt("Query failed or was canceled"), 'ok');
    }
    
  }
  $total_duration_formatted = number_format($total_duration/60, 2);
  drush_log(dt("Total duration: @total_duration minutes.", array('@total_duration' => $total_duration_formatted)), 'ok');
}

function drush_pbd_update_assessments() {
  _civicrm_init();
  $key =  'net.ourpowerbase.assessment';
  $ext_system = CRM_Extension_System::singleton();
  $manager = $ext_system->getManager();
  $status = $manager->getStatus($key);
  if($status != $manager::STATUS_INSTALLED) {
    drush_log(dt("The net.ourpowerbase.assess extension is not installed."), 'error');
    return;
  }
  if(!assessment_is_configured()) {
    drush_log(dt("The net.ourpowerbase.assess extension is installed, but not configured."), 'error');
    return;
  }
  $activity_contact_types = CRM_Core_OptionGroup::values('activity_contacts', FALSE, FALSE, FALSE, NULL, 'name');
  $target_id = CRM_Utils_Array::key('Activity Targets', $activity_contact_types);

  // List of contacts with at least one activity with a non-empty engagement level.
  $contacts_sql = "SELECT DISTINCT contact_id FROM civicrm_activity a JOIN ".
    "civicrm_activity_contact ac ON a.id = ac.activity_id WHERE ".
    "a.is_deleted = 0 AND a.engagement_level IS NOT NULL AND ac.record_type_id = %1";
  $contacts_params = array(1 => array($target_id, 'Integer'));

  // Query for getting the relevant engagement level
  $engagement_sql = "SELECT a.engagement_level, a.activity_date_time FROM civicrm_activity a ".
      "JOIN civicrm_activity_contact ac ON a.id = ac.activity_id ".
      "WHERE a.is_deleted = 0 AND a.engagement_level IS NOT NULL AND ".
      "a.activity_date_time < NOW() AND ac.contact_id = %1 ".
      "AND ac.record_type_id = %2 " .
      "ORDER BY activity_date_time DESC LIMIT 1";
  $contacts_dao = CRM_Core_DAO::executeQuery($contacts_sql, $contacts_params);
  while($contacts_dao->fetch()) {
    $engagement_params = array(
      1 => array($contacts_dao->contact_id, 'Integer'),
      2 => array($target_id, 'Integer')
    );
    $engagement_dao = CRM_Core_DAO::executeQuery($engagement_sql, $engagement_params);
    $engagement_dao->fetch();
    if($engagement_dao->N != 0) {
      drush_log("Updating contact ($contacts_dao->contact_id) with: $engagement_dao->activity_date_time and $engagement_dao->engagement_level\n");
      assessment_update_assessment(
        $contacts_dao->contact_id,
        $engagement_dao->engagement_level,
        $engagement_dao->activity_date_time
      );
    }
  }
}

/**
 * For the given contact id, output all subscription links.
 *
 * These are the links that a user can follow to update their
 * own subscription (recurring contribution).
 */
function drush_pbd_subscription_links() {
  _civicrm_init();
  $id = drush_get_option('id');

  if(empty($id)) {
    drush_log(dt("Please pass the contact id via the id parameter."), 'error');
    return false;
  }
  $contributionStatus = CRM_Contribute_PseudoConstant::contributionStatus(NULL, 'name');
  $in_progress_id = array_search('In Progress', $contributionStatus);

  $params = array(
    0 => array($in_progress_id, 'Integer'),
    1 => array($id, 'Integer')
  );

  // Try to find an in-progress recurring contribution matching this id.
  $sql = "SELECT c.id AS id, first_name, last_name, r.contribution_status_id FROM civicrm_contribution c
    JOIN civicrm_contribution_recur r ON c.contribution_recur_id = r.id
    JOIN civicrm_contact co ON r.contact_id = co.id
    WHERE r.contact_id = %1
    AND is_deceased = 0 
    ORDER BY r.id DESC LIMIT 1";
  $dao = CRM_Core_DAO::executeQuery($sql, $params);
  $dao->fetch();
  if($dao->N == 0) {
    drush_log("Failed to find a matching recurring contribution record for that id.", 'error');
    return;
  }
  $contribution_id = $dao->id;
  $entity = 'contribution';
  $paymentObject = CRM_Financial_BAO_PaymentProcessor::getProcessorForEntity($contribution_id, 'contribute', 'obj');
  $update_url = $paymentObject->subscriptionURL($contribution_id, $entity, 'update');
  $cancel_url = $paymentObject->subscriptionURL($contribution_id, $entity, 'cancel');
  $billing_url = $paymentObject->subscriptionURL($contribution_id, $entity, 'billing');
  drush_print("Name: " . $dao->first_name . " " . $dao->last_name . "(" . $dao->contribution_status_id . ")");
  drush_print("Update: $update_url");
  drush_print("Cancel: $cancel_url");
  drush_print("Billing: $billing_url");

}
/**
 * Prepare a new theme.
 *
 * When creating a new theme, a number of steps have to be taken to enable
 * it to work properly as a custom theme.
 */
function drush_pbd_prepare_theme() {
  // Clear the cache so the theme is visible.
  drupal_theme_rebuild();
  $themes = system_rebuild_theme_data();
  $name = drush_get_option('name');
  // Pass --default if you want this theme to replace the default theme.'
  // This is only useful on new sites.
  $default = drush_get_option('default');
  // Ensure the theme is here (e.g. typo)
  if(!array_key_exists($name, $themes)) {
    drush_log("That theme does not exist in the filesystem.", 'error');
    return;
  }
  // Enable it.
  theme_enable(array($name));
  // Save theme settings.
  $settings = array(
    'toggle_logo' => 1,
    'toggle_name' => 0,
    'toggle_slogan' => 0,
    'toggle_node_user_picture' => 0,
    'toggle_comment_user_verification' => 0,
    'toggle_favicon' => 0,
    'toggle_main_menu' => 0,
    'toggle_secondary_menu' => 0,
    'default_logo' => 0,
    'logo_path' => "sites/default/themes/${name}/${name}-logo.png"
  );
  variable_set('theme_' . $name . '_settings', $settings);

  // Now let's kill the blocks.
  db_update("block")
    ->fields(array('status' => 0, 'region' => -1))
    ->condition('theme', $name, '=')
    ->condition('region', 'content', '!=')
    ->execute();

  if ($default) {
    // Replace the "default" theme with this one.
    db_update("themekey_properties")
      ->fields(array('theme' => $name))
      ->condition('theme', 'default', '=')
      ->execute();
  }
}
/*
 * ***************************************************************
 *
 * Helper functions below
 *
 * ***************************************************************
 */

/**
 * Helper function for storing persistant data
 * for pbd.
 **/
function pbd_set_setting($key, $value) {
  $group = 'PowerBase Drush';
  pbd_setting($key, $value, $group);
}

/**
 * Helper function for getting persistant data
 * for this extension.
 **/
function pbd_get_setting($key, $default = NULL) {
  $group = 'PowerBase Drush';
  $codeVer = CRM_Utils_System::version();
  if(version_compare($codeVer, '4.7', '>=')) {
    $params['name'] = $key;
    $ret = civicrm_api3('Setting', 'getvalue', $params);
  }
  else {
    $ret = CRM_Core_BAO_Setting::getItem($group, $key);
  }
  if(empty($ret)) return $default;
  return $ret;
}

/**
 * Put in default ckeditor configuration.
 * Technically this should only go in sites that are 4.7 or higher
 * but there is no harm in having it land in 4.6 sites.
 **/
function pbd_configure_ckeditor() {
  $base_dir = __dir__ . '/templates';
  $source = $base_dir . '/crm-ckeditor-default.js';
  $dest = "/var/www/powerbase/sites/default/files/civicrm/persist/crm-ckeditor-default.js";
  copy($source, $dest);
}

/**
 * Add a "example-tag" to the installation
 *
 * Include/Exclude search fails if there are no tags so
 * add one test tag to avoid that problem.
 **/
function pbd_add_first_tag() {
  try {
    civicrm_api3('Tag', 'create', array(
      'sequential' => 1,
      'name' => "example-tag",
      'description' => "First tag for default install. If you are using tags, please delete. If you are not using tags, keep this around because certain features fail if no tags are present.",
    ));
  }
  catch (CiviCRM_API3_Exception $e) {
    if($e->getErrorCode() == 'already exists') {
      return TRUE;
    }
    drush_log(dt("Failed to add first tag."), 'error');
    drush_log($e->getMessage(), 'error');
    return FALSE;
  }
}

/**
 * Enable waitlist participant status types
 **/
function pbd_enable_waitlist_participant_status_types() {
  try {
    foreach(array('On waitlist', 'Pending from waitlist') as $name) {
      $params = array('return' => 'id', 'name' => $name);
      $results = civicrm_api3('ParticipantStatusType', 'get', $params);
      $values = array_pop($results['values']);
      $id = $values['id'];
      $params = array('id' => $id, 'is_active' => 1);
      civicrm_api3('ParticipantStatusType', 'create', $params);
    }
  }
  catch (CiviCRM_API3_Exception $e) {
    drush_log(dt("Failed to enable waitlist participant status type."), 'error');
    drush_log($e->getMessage(), 'error');
    return FALSE;
  }
  return TRUE;



}

/**
 * Don't warn groups about updates.
 *
 * This is sometimes false and we get alerts and handle the updates.
 **/
function pbd_disable_security_update_alert() {
  $codeVer = CRM_Utils_System::version();
  if(version_compare($codeVer, '4.7', '<')) {
    $name = 'securityUpdateAlert';
  }
  else {
    $name = 'securityAlert';
  }
  $params = array($name => 0);
  civicrm_api3('Setting', 'create', $params);
  return TRUE;
}

/**
 * Remove country from address formats
 *
 * Run once - most organizations don't work out of the country.
 **/
function pbd_remove_country_from_address_formats() {
  try {
    foreach(array('address_format', 'mailing_format') as $field) {
      $params = array('return' => $field);
      $results = civicrm_api3('Setting', 'get', $params);
      $values = array_pop($results['values']);
      $new = str_replace('{contact.country}', '', $values[$field]);
      $params = array($field => $new);
      civicrm_api3('Setting', 'create', $params);
    }
  }
  catch (CiviCRM_API3_Exception $e) {
    drush_log(dt("Failed to remove country from address formats."), 'error');
    drush_log($e->getMessage(), 'error');
    return FALSE;
  }
  return TRUE;
}
/**
 * Disable communication style
 *
 * This is a new field in 4.5 that allows you to set a contact to
 * "formal" or "informal" - we decided it is just clutter, so we
 * disable it by disabling all the available options.
 */
function pbd_disable_communication_style() {
  $sql = "UPDATE civicrm_option_value v JOIN civicrm_option_group g ON
    v.option_group_id = g.id SET v.is_active = 0 WHERE g.name = 'communication_style'";
  CRM_Core_DAO::executeQuery($sql);
}

/**
 * People often edit this profile without realizing how badly
 * is screws up new Drupal user creation,.
 **/
function pbd_make_drupal_user_profile_reserved() {
  $sql = "UPDATE civicrm_uf_group SET is_reserved = 1 WHERE id = 1";
  CRM_Core_DAO::executeQuery($sql);
}


/**
 * Change one side of certain relationships to differentiate them.
 *
 * It's impossible to differentiate sides to the spouse, partner
 * and sibling relationships, yet that is helpful because it's often
 * one side of the relationship that we're most interested in.
 *
 * This function updates the A side of spouse, partner and sibling from
 * "X of" to "X is" and keeps the B side "X of"
 */
function pbd_differentiate_sides_to_equal_relationships() {
  $rels_to_change = array('Spouse of' => 'Spouse is', 'Partner of' => 'Partner is', 'Sibling of' => 'Sibling is');
  while(list($old, $new) = each($rels_to_change)) {
    $sql = "UPDATE civicrm_relationship_type SET name_a_b = %0, label_a_b = %0 ".
      "WHERE name_a_b = %1";
    $params = array(
      0 => array($new, 'String'),
      1 => array($old, 'String')
    );
    CRM_Core_DAO::executeQuery($sql, $params);
  }
}
function pbd_is_localhost($host) {
  if($host == 'localhost') return true;
  return false;
}

/**
 * Enable default components for a pb site.
 *
 **/
function pbd_enable_pb_components() {
  _civicrm_init();
  $params = array(
    'enable_components' => array(
      'CiviContribute',
      'CiviEvent',
      'CiviMail',
      'CiviReport',
      'CiviCampaign'
    )
  );
  try {
    civicrm_api3('Setting', 'create', $params);
  }
  catch (CiviCRM_API3_Exception $e) {
    drush_log(dt("Failed to configure components."), 'error');
    drush_log($e->getMessage(), 'error');
    return;
  }
}

/*
 * Iterate over all contacts without an addressee_id, email_greeting_id, and
 * postal_greeting_id and fill in default values
 */
function pbd_update_greetings() {
  _civicrm_init();
  $greetings = array('addressee','email_greeting','postal_greeting');
  require_once 'CRM/Core/OptionGroup.php';
  while(list(,$greeting) = each($greetings)) {
    drush_print("Running for $greeting.");
    // usually we want greeting id one, but as a sanity check, make sure that's
    // an option
    // filter = 1 is to filter by Individual contact type
    $ids = CRM_Core_OptionGroup::values( $greeting, null, null, null, "AND ( filter = 1 OR filter = 0 )", "value");
    if(!in_array(1, $ids)) {
      drush_print("The id 1 is not a valid greeting id for '$greeting'.");
      continue;
    }
    $id_field = $greeting . '_id';
    $display_field = $greeting . '_display';
    $display_value = "CONCAT('Dear ', first_name)";
    if($greeting == 'addressee') $display_value = 'display_name';
    $sql = "UPDATE civicrm_contact SET $display_field = $display_value, $id_field = 1 WHERE $id_field IS NULL";
    CRM_Core_DAO::executeQuery( $sql );
  }
}

function pbd_lock_down_user_registration() {
  variable_set('user_register',0);
}

function pbd_add_message_templates($add_templates = NULL, $disable_templates = NULL) {
  _civicrm_init();
  // Get templates to add.
  if(is_null($add_templates)) {
    $add_templates = pbd_get_templates();
  }

  if(is_null($disable_templates)) {
    // Set templates to disable.
    $disable_templates = array(
      'Sample CiviMail Newsletter Template',
      'Sample Responsive Design Newsletter - Single Column Template',
      'Sample Responsive Design Newsletter - Two Column Template',
      'Powerbase Mail Template v1',
    );
  }

  // Get existing templates so we don't add duplicates
  $params = array('rowCount' => 0);
  try {
    $existing_result = civicrm_api3('messageTemplate', 'get', $params);
  }
  catch (CiviCRM_API3_Exception $e) {
    drush_log(dt("Failed to get existing message templates."), 'error');
    drush_log($e->getMessage(), 'error');
    return;
  }

  $existing_templates = $existing_result['values'];

  // Now keep track of what to add and what to disable.
  $disable_ids = array();
  while(list(,$values) = each($existing_templates)) {
    $title = $values['msg_title'];
    if(array_key_exists($title,$add_templates)) {
      // Remove from list of templates to add below.
      unset($add_templates[$title]);
    }
    if(in_array($title, $disable_templates) && $values['is_active'] == 1) {
      // Disable this one.
      $params = array('id' => $values['id'], 'is_active' => 0);
      civicrm_api3('messageTemplate', 'update', $params);
    }
  }

  // Now add templates...
  while(list(,$params) = each($add_templates)) {
    drush_log(dt("Adding @t",array('@t'=>$params['msg_title'])), 'notice');
    try {
      $existing_result = civicrm_api3('messageTemplate', 'create', $params);
    }
    catch (CiviCRM_API3_Exception $e) {
      drush_log(dt("Failed to create message template."), 'error');
      drush_log($e->getMessage(), 'error');
      return;
    }
  }
}

function pbd_get_templates() {
  // use msg_title as array key to help with dupe checking when adding
  $base_dir = __dir__ . '/templates';
  return array(
    'Powerbase Responsive Email One Column' => array(
      'msg_title' => 'Powerbase Responsive Email One Column',
      'msg_subject' => '',
      'msg_text' => '',
      'msg_html' => file_get_contents($base_dir . '/powerbase-responsive-one-column.html'),
      'is_active' => 1,
    ),
    'Powerbase Responsive Email Two Column' => array(
      'msg_title' => 'Powerbase Responsive Email Two Column',
      'msg_subject' => '',
      'msg_text' => '',
      'msg_html' => file_get_contents($base_dir . '/powerbase-responsive-two-column.html'),
      'is_active' => 1,
    ),
    'Powerbase Responsive Email One Column No Graphics' => array(
      'msg_title' => 'Powerbase Responsive Email One Column No Graphics',
      'msg_subject' => '',
      'msg_text' => '',
      'msg_html' => file_get_contents($base_dir . '/powerbase-responsive-one-column-no-graphics.html'),
      'is_active' => 1,
    ),
    'Powerbase Thank You Letter Template v1' => array(
      'msg_title' => 'Powerbase Thank You Letter Template v1',
      'msg_subject' => '',
      'msg_text' => '',
      'msg_html' => file_get_contents($base_dir . '/email-thank-you-template.html'),
      'is_active' => 1,
    ),
  );

}

function pbd_set_blocks_for_theme($theme) {
  // Set default names for regions. Note: in the database, setting the
  // region to -1 disables it.
  $regions = array(
    'left_sidebar' => 'left_sidebar',
    'disabled' => -1,
    'footer' => 'footer',
    'content' => 'content'
  );

  // Now, override for specific theme names
  // for the same regions

  // Newsflash region names are a little different
  // then the generic ones.
  if($theme == 'newsflash') {
    $regions['left_sidebar'] = 'sidebar_first';
  }

  // The iframe theme should have all blocks disabled
  // except the content block. Otherwise, it's the same
  // as newsflash (because it's a sub theme)
  if($theme == 'iframe') {
    $regions['left_sidebar'] = -1;
    $regions['footer'] = -1;
  }

  // responsive_bartik / powerbase
  if($theme == 'powerbase') {
    $regions['left_sidebar'] = 'sidebar_first';
  }

  // Iterate over every standard block and define
  // how it should be displayed.
  $blocks = array(
    0 => array(
      ':region' => $regions['left_sidebar'],
      ':status' => 1,
      ':module' => 'system',
      ':delta' => 'management',
      ':weight' => '-10',
      ':title' => '',
    ),
    1 => array(
      ':region' => $regions['left_sidebar'],
      ':status' => 1,
      ':module' => 'civicrm',
      ':delta' => 1,
      ':weight' => '-14',
      ':title' => '',
    ),
    2 => array(
      ':region' => $regions['left_sidebar'],
      ':status' => 1,
      ':module' => 'civicrm',
      ':delta' => 2,
      ':weight' => '-13',
      ':title' => '',
    ),
    3 => array(
      ':region' => $regions['disabled'],
      ':status' => 0,
      ':module' => 'civicrm',
      ':delta' => 3,
      ':weight' => '-12',
      ':title' => '',
    ),
    4 => array(
      ':region' => $regions['disabled'],
      ':status' => 0,
      ':module' => 'civicrm',
      ':delta' => 4,
      ':weight' => '-12',
      ':title' => '',

    ),
    5 => array(
      ':region' => $regions['disabled'],
      ':status' => 0,
      ':module' => 'civicrm',
      ':delta' => 5,
      ':weight' => '-10',
      ':title' => '',

    ),
    6 => array(
      ':region' => $regions['disabled'],
      ':status' => 0,
      ':module' => 'civicrm',
      ':delta' => 1,
      ':weight' => '-9',
      ':title' => '',

    ),
    7 => array(
      ':region' => $regions['disabled'],
      ':status' => 0,
      ':module' => 'civicrm',
      ':delta' => 1,
      ':weight' => '-11',
      ':title' => '',

    ),
    8 => array(
      ':region' => $regions['footer'],
      ':status' => 1,
      ':module' => 'opb',
      ':delta' => 'bestviewed',
      ':weight' => '-13',
      ':title' => '',

    ),
    9 => array(
      ':region' => $regions['left_sidebar'],
      ':status' => 1,
      ':module' => 'system',
      ':delta' => 'help',
      ':weight' => '-9',
      ':title' => '',

    ),
    10 => array(
      ':region' => $regions['content'],
      ':status' => 1,
      ':module' => 'system',
      ':delta' => 'main',
      ':weight' => '-9',
      ':title' => '',

    ),
    11 => array(
      ':region' => $regions['disabled'],
      ':status' => 0,
      ':module' => 'system',
      ':delta' => 'main-menu',
      ':weight' => '-6',
      ':title' => '',

    ),
    12 => array(
      ':region' => $regions['left_sidebar'],
      ':status' => 1,
      ':module' => 'system',
      ':delta' => 'management',
      ':weight' => '-11',
      ':title' => '',

    ),
    13 => array(
      ':region' => $regions['disabled'],
      ':status' => 0,
      ':module' => 'system',
      ':delta' => 'navigation',
      ':weight' => '-5',
      ':title' => '',

    ),
    14 => array(
      ':region' => $regions['disabled'],
      ':status' => 0,
      ':module' => 'system',
      ':delta' => 'powered-by',
      ':weight' => '-4',
      ':title' => '',

    ),
    15 => array(
      ':region' => $regions['disabled'],
      ':status' => 0,
      ':module' => 'system',
      ':delta' => 'user-menu',
      ':weight' => '1',
      ':title' => '',

    ),
    16 => array(
      ':region' => $regions['left_sidebar'],
      ':status' => 1,
      ':module' => 'user',
      ':delta' => 'login',
      ':weight' => '-8',
      ':title' => '',

    ),
    17 => array(
      ':region' => $regions['disabled'],
      ':status' => 0,
      ':module' => 'user',
      ':delta' => 'new',
      ':weight' => '2',
      ':title' => '',

    ),
    18 => array(
      ':region' => $regions['disabled'],
      ':status' => 0,
      ':module' => 'user',
      ':delta' => 'online',
      ':weight' => '3',
      ':title' => '',

    ),
    19 => array(
      ':region' => $regions['left_sidebar'],
      ':status' => 1,
      ':module' => 'opb',
      ':delta' => 'phone_email_search',
      ':weight' => '3',
      ':title' => '',
    ),
    20 => array(
      ':region' => $regions['left_sidebar'],
      ':status' => 1,
      ':module' => 'locale',
      ':delta' => 'language',
      ':weight' => '-11',
      ':title' => '',
    ),
  );

  while(list(,$block) = each ($blocks)) {

    $sql = "SELECT bid FROM {block} WHERE theme = :theme AND ".
      "module = :module AND delta = :delta";
    // add theme
    $block[':theme'] = $theme;

    $params = $block;
    // unset unused params for select query
    unset($params[':region']);
    unset($params[':status']);
    unset($params[':weight']);
    unset($params[':title']);

    $result = db_query($sql, $params);

    reset($block);
    $fields = array();
    while(list($k,$v) = each($block)) {
      $field = trim($k,':');
      $fields[$field] = $v;
    }
    $record = $result->fetch();
    if(empty($record)) {
      $fields['pages'] = '';
      db_insert('block')
        ->fields($fields)
        ->execute();
    } else {
      db_update('block')
        ->fields($fields)
        ->condition('bid', $record->bid, '=')
        ->execute();
    }
    db_query($sql, $params);
    // Seems to be bug in some versions of drush
    if(function_exists('drush_cache_clear_block')) {
      drush_cache_clear_block();
    }
    else {
      cache_clear_all(NULL, 'cache_block');
    }
  }
}

function pbd_grant_admin_all_perms() {
  // heavy handed approach to compensate for a drupal core bug:
  // https://drupal.org/node/1454686
  // https://drupal.org/node/1153072

  // ensure the admin role is created and assigned
  if (!$rid = variable_get('user_admin_role')) {
    // this should be set
    $roles = user_roles();

    $role_name = 'administrator';
    $rid = array_search($role_name, $roles);
    if(empty($rid)) {
      // create an administrator role (could be new site)
      $role_obj = new stdClass();
      $role_obj->name = $role_name;
      user_role_save($role_obj);
      $role_obj = user_role_load_by_name($role_name);
      $rid = $role_obj->rid;
    }
  }

  // Create list of permissions that not even admins should have
  // access to on a production site.
  $exceptions = array('view report sql');
  // ensure the admin role has all avaialble permissions
  // $all_permissions = drupal_map_assoc(array_keys(module_invoke_all('permission')));
  $all_permissions = array_keys(module_invoke_all('permission'));
  $admin_permissions = array();
  while(list(,$perm) = each($all_permissions)) {
    if (in_array($perm, $exceptions)) {
      continue;
    }
    $admin_permissions[$perm] = true;
  }
  user_role_change_permissions($rid, $admin_permissions);
}

function pbd_create_drupal_roles() {
  $roles = array(
    'Powerbase Administrator',
    'Powerbase User'
  );
  // get an array of all civi defined permissions

  $civi_perms = civicrm_permission();

  // Don't grant access to "access all custom data"
  // This perm will be granted to anon and authenticated
  // users below, which will grant everyone access. However,
  // some sites may want to remove this priv so ACLs can
  // do a more fine tuned approach.
  unset($civi_perms['access all custom data']);

  // There are some permissions that nobody should get on a production site.
  $never_access_perms = array('view report sql');
  foreach($never_access_perms as $never_access_perm) {
    unset($civi_perms[$never_access_perm]);
  }
  // Let's get admin too.
  $never_access_roles = array_merge($roles, array('administrator'));
  foreach($never_access_roles as $never_access_role) {
    if(FALSE !== ($role_obj = user_role_load_by_name($never_access_role))) {
      user_role_revoke_permissions($role_obj->rid, $never_access_perms);
    }
  }

  $civi_perms = array_keys($civi_perms);

  // Powerbase Admins  get all civi perms
  $perms['Powerbase Administrator'] = $civi_perms;

  // Powerbase Users get all with some exceptions
  $perms['Powerbase User'] = $civi_perms;
  $user_exceptions = array(
    'administer reserved groups',
    'adminster reserved tags',
    'administer dedupe rules',
    'administer reserved reports',
    'delete in CiviContribute',
    'administer payment processors',
    // VSEA has a custom extension to provide a permission to access
    // the download data permission.
    'access export menu',
  );

  // Ensure they are deleted if they have been granted in the past
  if(FALSE !== ($role_obj = user_role_load_by_name('Powerbase User'))) {
    user_role_revoke_permissions($role_obj->rid, $user_exceptions);
  }
  // Remove them from the list to grant access to
  while(list(,$exception) = each($user_exceptions)) {
    if(false !== ($key = array_search($exception, $perms['Powerbase User']))) {
      unset($perms['Powerbase User'][$key]);
    }
  }

  // not sure if this is necessary but shouldn't hurt
  $perms['Powerbase Administrator'][] = 'access content';
  $perms['Powerbase User'][] = 'access content';

  // Admins should have access to the admin area
  $perms['Powerbase Administrator'][] = 'access administration pages';
  $perms['Powerbase Administrator'][] = 'access content overview';

  // Set paths
  $perms['Powerbase Administrator'][] = 'create url aliases';

  // Ability to enable modles
  $perms['Powerbase Administrator'][] = 'administer modules';

  // Admins should be able to have full control over all drupal content
  $perms['Powerbase Administrator'][] = 'administer nodes';

  // Admin should be able to modify drupal users
  $perms['Powerbase Administrator'][] = 'administer permissions';
  $perms['Powerbase Administrator'][] = 'administer users';
  $perms['Powerbase Administrator'][] = 'access administration pages';

  // Make sure Powerbase admins get civicrm_cicero privs
  if(module_exists('civicrm_cicero')) {
    $perms['Powerbase Administrator'][] = 'administer civicrm cicero';
  }

  // Full access to views
  $perms['Powerbase Administrator'][] = 'administer views';
  $perms['Powerbase Administrator'][] = 'access all views';

  // Full access to webform
  $perms['Powerbase Administrator'][] = 'create webform content';
  $perms['Powerbase Administrator'][] = 'edit own webform content';
  $perms['Powerbase Administrator'][] = 'edit any webform content';
  $perms['Powerbase Administrator'][] = 'delete any webform content';
  $perms['Powerbase Administrator'][] = 'delete own webform content';
  $perms['Powerbase Administrator'][] = 'access all webform results';
  $perms['Powerbase Administrator'][] = 'access own webform results';
  $perms['Powerbase Administrator'][] = 'edit all webform submissions';
  $perms['Powerbase Administrator'][] = 'delete all webform submissions';
  $perms['Powerbase Administrator'][] = 'access own webform submissions';
  $perms['Powerbase Administrator'][] = 'edit own webform submissions';
  $perms['Powerbase Administrator'][] = 'delete own webform submissions';

  // Ability to get backups
  if(module_exists('backup_migrate')) {
    $perms['Powerbase Administrator'][] = 'access backup and migrate';
    $perms['Powerbase Administrator'][] = 'perform backup';
    $perms['Powerbase Administrator'][] = 'access backup files';
    $perms['Powerbase Administrator'][] = 'delete backup files';
  }

  // Admins should have themekey admin privs
  $perms['Powerbase Administrator'][] = 'administer theme assignments';
  $perms['Powerbase Administrator'][] = 'administer themekey settings';

  // And ability to access theme area
  $perms['Powerbase Administrator'][] = 'administer themes';
  
  while(list(,$role) = each($roles)) {
    $role_obj = user_role_load_by_name($role);
    if(!$role_obj) {
      // add the role
      $role_obj = new stdClass();
      $role_obj->name = $role;
      user_role_save($role_obj);
    }
    user_role_grant_permissions($role_obj->rid,$perms[$role]);
  }
}

function pbd_grant_anonymous_and_authenticated_role_access($one_time = FALSE) {
  $default_anon_perms = array(
    'access CiviMail subscribe/unsubscribe pages',
    'make online contributions',
    'register for events',
    'view event info',
    'view public CiviMail content',
    'sign CiviCRM Petition',
    'profile create',
    'access all custom data',
    'access content',
    'access uploaded files'
  );

  // Unset some permissions that should only be set on the first time
  // we run (so they can be un-checked on a site-by-site basis
  if(!$one_time) {
    // Some sites use ACLs to more minutely provide access to
    // custom data
    $key = array_search('access all custom data', $default_anon_perms);
    if($key) {
      unset($default_anon_perms[$key]);
    }
  }
  // auth perms are same with one addition.
  $default_auth_perms = $default_anon_perms;
  $default_auth_perms[] = 'access Contact Dashboard';
  // ensure we are not setting permissions that don't exist

  $module_perms = user_permission_get_modules();
  $available_perms = array_keys($module_perms);
  $problem_anon_perms = array_diff($default_anon_perms,$available_perms);
  $key = null;
  while(list(,$perm) = each($problem_anon_perms)) {
    drush_log("Unsetting anon: $perm");
    $key = array_search($perm, $default_anon_perms);
    if($key || $key == 0) unset($default_anon_perms[$key]);
  }
  $key = null;
  $problem_auth_perms = array_diff($default_auth_perms,$available_perms);
  while(list(,$perm) = each($problem_auth_perms)) {
    drush_log("Unsetting auth perm: $perm");
    $key = array_search($perm, $default_auth_perms);
    if($key || $key == 0) unset($default_auth_perms[$key]);
  }
  $role_obj = user_role_load_by_name('anonymous user');
  user_role_grant_permissions($role_obj->rid,$default_anon_perms);
  $role_obj = user_role_load_by_name('authenticated user');
  user_role_grant_permissions($role_obj->rid,$default_auth_perms);
}

// designed to be used in array_walk function below,
// it strips nbsp; from the value passed in
function opb_strip_nbsp(&$value, $key) {
  $value = str_replace('&nbsp;','',$value);
}

function pbd_disable_create_new_household_menu() {
  $delete = array('New Household' => 'civicrm/contact/add?reset=1&ct=Household');
  $domainID  = CRM_Core_Config::domainID( );
  while(list($label, $url) = each($delete)) {
    $sql = "DELETE FROM civicrm_navigation WHERE label = %1 AND url = %2 ".
      "AND is_active = 1 AND domain_id = %3";
    $params = array(
      1 => array(
        $label, 'String'
      ),
      2 => array(
        $url, 'String'
      ),
      3 => array(
        $domainID, 'Integer'
      ),
    );
    CRM_Core_DAO::executeQuery( $sql, $params );
  }
}

function pbd_activate_cron_jobs() {
  $activate = array('process_mailing', 'process_membership', 'fetch_bounces', 'send_reminder', 'fetch_activities', 'cleanup', 'update_greeting', 'update_greeting', 'process_participant', 'process_sms', 'process_respondent', 'geocode', 'update_email_resetdate', 'disable_expired_relationships');
  $disable = array('version_check');
  _civicrm_init();
  require_once('CRM/Core/BAO/Job.php');
  // enable all cron jobs that should be enabled but are not enabled
  while(list(,$api_action) = each($activate)) {
    $params = array('api_action' => $api_action, 'is_active' => 0);
    $defaults = array();
    $job = CRM_Core_BAO_Job::retrieve( $params, $defaults );
    if(!empty($job) && $job->id) {
      CRM_Core_BAO_Job::setIsActive( $job->id, 1);
    }
  }
  while(list(,$api_action) = each($disable)) {
    $params = array('api_action' => $api_action, 'is_active' => 1);
    $defaults = array();
    $job = CRM_Core_BAO_Job::retrieve( $params, $defaults );
    if(!empty($job) && $job->id) {
      CRM_Core_BAO_Job::setIsActive( $job->id, 0);
    }
  }
  // Parameters should be set for update_email_resetdate. Make sure we don't have the default
  // broken parameters.
  $params = array('api_action' => 'update_email_resetdate', 'parameters' => 'minDays, maxDays=Consider mailings that have completed between minDays and maxDays');
  $defaults = array();
  $job = CRM_Core_BAO_Job::retrieve( $params, $defaults );
  if(!empty($job) && $job->id) {
    CRM_Core_DAO::setFieldValue( 'CRM_Core_DAO_Job', $job->id, 'parameters', "minDays=0\nmaxDays=10\n" );
  }
  // Parameters should be set for update_greeting (note: we are *only* updating addressee automatically). If we don't
  // have any parameters, it won't run.
  $params = array('api_action' => 'update_greeting');
  $defaults = array();
  $job = CRM_Core_BAO_Job::retrieve( $params, $defaults );
  if(!empty($job) && $job->id) {
    CRM_Core_DAO::setFieldValue( 'CRM_Core_DAO_Job', $job->id, 'parameters', "ct=Individual\ngt=addressee\n" );
  }
  // Parameters should be empty for geocoding. CiviCRM ships with sample params that will prevent it from
  // running properly.
  $params = array('api_action' => 'geocode');
  $defaults = array();
  $job = CRM_Core_BAO_Job::retrieve( $params, $defaults );
  if(!empty($job) && $job->id) {
    CRM_Core_DAO::setFieldValue( 'CRM_Core_DAO_Job', $job->id, 'parameters', "" );
  }
  // Sending email should be set to Always (everytime cron is run) not hourly
  // (which is the default)
  $params = array('api_action' => 'process_mailing', 'run_frequency' => 'Hourly');
  $defaults = array();
  $job = CRM_Core_BAO_Job::retrieve( $params, $defaults );
  if(!empty($job) && $job->id) {
    CRM_Core_DAO::setFieldValue( 'CRM_Core_DAO_Job', $job->id, 'run_frequency', 'Always' );
  }
  // Sending reminders should go out hourly rather than daily. If it's daily and you
  // ask for a reminder to go out 8 hours before the event, you may miss the window
  // to send it.
  $params = array('api_action' => 'send_reminder', 'run_frequency' => 'Daily');
  $defaults = array();
  $job = CRM_Core_BAO_Job::retrieve( $params, $defaults );
  if(!empty($job) && $job->id) {
    CRM_Core_DAO::setFieldValue( 'CRM_Core_DAO_Job', $job->id, 'run_frequency', 'Hourly' );
  }

  // fetch activities on each run rather than hourly
  // https://support.ourpowerbase.net/ticket/1806
  $params = array('api_action' => 'fetch_activities', 'run_frequency' => 'Hourly');
  $defaults = array();
  $job = CRM_Core_BAO_Job::retrieve( $params, $defaults );
  if(!empty($job) && $job->id) {
    CRM_Core_DAO::setFieldValue( 'CRM_Core_DAO_Job', $job->id, 'run_frequency', 'Always' );
  }
}

function pbd_change_debugging($status) {
  _civicrm_init();
  require_once "CRM/Core/BAO/ConfigSetting.php";
  $params = Array ( 'debug' => $status, 'backtrace' => $status);
  CRM_Core_BAO_ConfigSetting::add($params);
}

function pbd_set_optin_defaults() {
  pbd_setting('profile_add_to_group_double_optin', '0', 'Mailing Preferences');
  pbd_setting('profile_double_optin', '0', 'Mailing Preferences');
}

function pbd_default_geo_settings() {
  _civicrm_init();
  $mapProvider = 'Google';
  $geoProvider = 'Google';
  pbd_setting('mapProvider', $mapProvider);
  pbd_setting('geoProvider', $geoProvider);
}

/**
 * set values for the iframe theme - designed to be displayed
 * with no headers, footers, menus etc.
 **/
function pbd_set_iframe_settings() {
  $theme_iframe_settings = variable_get('theme_iframe_settings', array());
  $theme_iframe_settings['newsflash_banner'] = 0;
  $theme_iframe_settings['toggle_logo'] = 0;
  $theme_iframe_settings['toggle_name'] = 0;
  $theme_iframe_settings['toggle_favicon'] = 0;
  $theme_iframe_settings['toggle_main_menu'] = 0;
  $theme_iframe_settings['toggle_secondary_menu'] = 0;
  $theme_iframe_settings['default_logo'] = 0;
  $theme_iframe_settings['newsflash_breadcrumb'] = 0;
  $theme_iframe_settings['newsflash_width'] = '90%';
  $theme_iframe_settings['newsflash_leftsidebarwidth'] = 0;
  $theme_iframe_settings['newsflash_rightsidebarwidth'] = 0;
  $theme_iframe_settings['newsflash_logowidth'] = 0;
  $theme_iframe_settings['newsflash_logoheight'] = 0;
  $theme_iframe_settings['favicon_path'] = '';
  variable_set('theme_iframe_settings', $theme_iframe_settings);
}

function pbd_set_powerbase_theme_settings() {
  $settings = variable_get('theme_powerbase_settings', array());
  // override the default favicon
  $settings['toggle_favicon'] = 1;
  $settings['default_favicon'] = 0;
  $settings['favicon_path'] = 'sites/all/themes/powerbase/images/poweron.ico';
  $settings['favicon_mimetype'] = 'image/vnd.microsoft.icon';

  // The breadcrumb uses real estate... but is also quite useful.
  $settings['bootstrap_breadcrumb'] = 1;
  // and override the default logo
  $settings['default_logo'] = 0;
  $settings['logo_path'] = 'sites/all/themes/powerbase/images/poweron.png';
  $settings['toggle_logo'] = 1;
  $settings['toggle_name'] = 1;
  $settings['toggle_slogan'] = 0;
  $settings['toggle_node_user_picture'] = 0;
  $settings['toggle_comment_user_picture'] = 0;
  $settings['toggle_comment_user_verification'] = 0;
  $settings['toggle_main_menu'] = 0;
  $settings['toggle_secondary_menu'] = 0;
  variable_set('theme_powerbase_settings', $settings);
}

function pbd_set_newsflash_settings() {
  $theme_newsflash_settings = variable_get('theme_newsflash_settings', array());
  // the newsflash banner is the "made by roople" thing in the footer
  $theme_newsflash_settings['newsflash_banner'] = 0;

  // these lines add our custom css stuff
  $theme_newsflash_settings['newsflash_uselocalcontent'] = 1;
  $theme_newsflash_settings['newsflash_localcontentfile'] = 'sites/all/themes/pb_custom_theme_elements/pb-newsflash.css';

  // override the default favicon
  $theme_newsflash_settings['toggle_favicon'] = 1;
  $theme_newsflash_settings['default_favicon'] = 0;
  $theme_newsflash_settings['favicon_path'] = 'sites/all/themes/pb_custom_theme_elements/poweron.ico';
  $theme_newsflash_settings['favicon_mimetype'] = 'image/vnd.microsoft.icon';

  // and override the default logo
  $theme_newsflash_settings['default_logo'] = 0;
  $theme_newsflash_settings['logo_path'] = 'sites/all/themes/pb_custom_theme_elements/poweron.png';

  // who doesn't want a breadcrumb?
  $theme_newsflash_settings['newsflash_breadcrumb'] = 1;
  variable_set('theme_newsflash_settings', $theme_newsflash_settings);
}

function pbd_set_default_civicrm_option_lists() {
  pbd_set_default_preferred_languages();
  pbd_set_prefixes();
  pbd_set_suffixes();
  pbd_set_elected_levels();
}

function pbd_set_elected_levels() {
  $options = array(
    'City Council' => 'City Council',
    'City Comptroller' => 'City Comptroller',
    'Public Advocate' => 'Public Advocate' ,
    'Borough' => 'Borough',
    'U.S. House of Representatives' => 'U.S. House of Representatives',
    'U.S. Senate' => 'U.S. Senate',
    'State Assembly' => 'State Assembly',
    'State Comptroller' => 'State Comptroller',
    'State Senate' => 'State Senate'
  );
  asort($options);

  $option_group_name = 'elected_level_options';
  pbd_create_options($option_group_name, $options);
}

function pbd_set_prefixes() {
  $options = array('Mr.', 'Ms.','Mrs.','Dr.','Ald.', 'Asm.', 'Assembly Member', 'Assemblyman', 'Assemblywoman', 'Bishop', 'Borough President', 'Br', 'Brother', 'Capt.', 'Coach', 'Commissioner', 'Comptroller', 'Congressman', 'Congresswoman', 'Council Member', 'Councilman', 'Councilwoman', 'Deacon', 'Dean', 'Det.', 'Father', 'Fr.', 'Gov.', 'Governor', 'Hon.', 'Imam', 'Lt. Governor', 'Mayor', 'Minister', 'Monsignor',  'Mr. & Mrs.', 'Mr. & Ms.', 'Mrs. & Mr.', 'Ms. & Mr.', 'Msgr.', 'Pastor', 'President', 'Prof.', 'Professor', 'Rabbi', 'Rep.', 'Representative', 'Rev.', 'Rev. Canon', 'Rev. Deacon', 'Rev. Dr.', 'Reverend', 'Sen.', 'Senator', 'Sgt', 'Sister', 'Sr.', 'Sra.', 'The Honorable', 'The Venerable');
  sort($options);
  $option_group_name = 'individual_prefix';
  pbd_create_options($option_group_name, $options);
}

function pbd_set_suffixes() {
  $options = array('Jr.', 'Sr.','II', 'III', 'IV','V','VI','VII','AIA', 'CPA', 'CRNP', 'CSW', 'DDS', 'DPA', 'DPM', 'DSW', 'JD', 'Esq.', 'LFD', 'LMSW', 'LPC', 'M.D.', 'M.Div.', 'MHS', 'MPA', 'MSW', 'OFM', 'OP', 'PE', 'Ph.D', 'RSM', 'RN', 'S.J.',);
  sort($options);
  $option_group_name = 'individual_suffix';
  pbd_create_options($option_group_name, $options);
}

function pbd_create_options($option_group_name, $options) {
  _civicrm_init();
  require_once('api/api.php');

 $results = civicrm_api("OptionGroup","get", array ('version' => 3,'sequential' =>1, 'name' => $option_group_name ));
  if($results['is_error'] == 0) {
    $values = array_pop($results['values']);
    if(empty($values) || !array_key_exists('id', $values)) {
      drush_log(dt("Option group does not exist: @option_group_name, creating it.", array('@option_group_name' => $option_group_name)),'notice');
      $params = array('version' => 3, 'name' => $option_group_name);
      $results = civicrm_api('OptionGroup', 'create', $params);
      if($results['is_error'] != 0) {
        drush_log(dt("Failed to create option group: @option_group_name, creating it.", array('@option_group_name' => $option_group_name)),'error');
        return FALSE;
      }
      $values = array_pop($results['values']);
    }
    $id = $values['id'];

    // blow away existing params so we can be sure the weight will be set properly
    $sql = "DELETE FROM civicrm_option_value WHERE option_group_id = %0";
    $query_params = array ( 0 => array($id, 'Integer'));
    $dao = CRM_Core_DAO::executeQuery($sql, $query_params);

    $params = array(
      'version' => 3,
      'option_group_id' => $id,
    );
    $weight = 0;
    while(list($k,$value) = each($options)) {
      if(is_numeric($k)) {
        // not a k=>v pair, just use the value for the label
        $params['label'] = $value;
      } else {
        // is a k => v pair
        $params['label'] = $k;
        $params['value'] = $value;
        $params['weight'] = $weight;
        $weight++;
      }
      civicrm_api("OptionValue","create", $params);
    }
  }
}

function pbd_set_default_preferred_languages() {
  _civicrm_init();
  $include = "('en_US','es_MX','zh_CN', 'ar_EG', 'lo_LA')";
  $sql = "SELECT id FROM civicrm_option_group WHERE name = 'languages'";
  $dao = CRM_Core_DAO::executeQuery($sql);
  while($dao->fetch()) {
    $option_group_id = $dao->id;
  }
  $params = array( 0 => array($option_group_id, 'Integer'));
  $sql = "UPDATE civicrm_option_value SET is_active = 0 WHERE option_group_id = %0 ".
    "AND is_active = 1 AND name NOT IN $include";
  $dao = CRM_Core_DAO::executeQuery($sql, $params);

}

function pbd_adjust_display_preferences() {
  pbd_fix_contact_edit_options();
  pbd_fix_user_dashboard_options();

}

function pbd_fix_contact_edit_options() {
  _civicrm_init();

  // Get the id of the option group
  $results = civicrm_api("OptionGroup","get", array ('version' => 3,'name' => 'contact_edit_options' ));

  // Recreate options so we can ensure proper weight
  if($results['is_error'] == 0) {
    $values = array_pop($results['values']);
    $id = $values['id'];

    // Now initialize new $values array to be used for the option values.
    $values = array(
      'Address' => array(
        'label' => 'Address',
        'weight' => 6,
        'filter' => 0,
      ),
      'CommunicationPreferences' => array(
        'label' => 'Communication Preferences',
        'weight' => 7,
        'filter' => 0,
      ),
      'Demographics' => array(
        'label' => 'Demographics',
        'weight' => 8,
        'filter' => 0,
      ),
      'CustomData' => array(
        'label' => 'Custom Data',
        'weight' => 9,
        'filter' => 0,
      ),
      'Notes' => array(
        'label' => 'Notes',
        'weight' => 10,
        'filter' => 0,
      ),
      'TagsAndGroups'  => array(
        'label' => 'Tags and Groups',
        'weight' => 11,
        'filter' => 0,
      ),
      'Email' => array(
        'label' => 'Email',
        'weight' => 12,
        'filter' => 1,
      ),
      'Phone' => array(
        'label' => 'Phone',
        'weight' => 12,
        'filter' => 1,
      ),
      'IM' => array(
        'label' => 'IM',
        'weight' => 13,
        'filter' => 1,
      ),
      'Website' => array(
        'label' => 'Website',
        'weight' => 14,
        'filter' => 1,
      ),
      'OpenID' => array(
        'label' => 'Open ID',
        'weight' => 14,
        'filter' => 1,
      ),
    );
    // FIXME: this should be removed after the 4.5 upgrade
    $dbVer = db_query('select version from {civicrm_domain} where id = :id', array(':id' => CIVICRM_DOMAIN_ID))->fetchField();
    if (substr($dbVer, 0, 3) >= '4.5') {
      // These are extra options added in 4.5
      $four_five_values = array(
        'Prefix' => array(
          'label' => 'Prefix',
          'weight' => 15,
          'filter' => 2,
        ),
        'Formal Title' => array(
          'label' => 'Formal Title',
          'weight' => 16,
          'filter' => 2,
        ),
        'First Name' => array(
          'label' => 'First Name',
          'weight' => 17,
          'filter' => 2,
        ),
        'Middle Name' => array(
          'label' => 'Middle Name',
          'weight' => 18,
          'filter' => 2,
        ),
        'Last Name' => array(
          'label' => 'Last Name',
          'weight' => 19,
          'filter' => 2,
        ),
        'Suffix' => array(
          'label' => 'Suffix',
          'weight' => 20,
          'filter' => 2,
        ),
      );
      $values = array_merge($values, $four_five_values);
    }

    // Resave all values.
    $params = array(
      'version' => 3,
      'option_group_id' => $id,
    );

    // Keep an array of ids - we'll need to that to save below when we indicate
    // which of these options should be displayed (the option value indicates the
    // weight, not whether or not to display them).
    $ids = array();
    // re-create each param
    while(list($k,$v) = each($values)) {
      // blow away existing item
      $sql = "DELETE FROM civicrm_option_value WHERE option_group_id = %0 AND name = %1";
      $query_params = array ( 0 => array($id, 'Integer'), 1 => array($k, 'String'));
      CRM_Core_DAO::executeQuery($sql, $query_params);

      $params['weight'] = $v['weight'];
      $params['name'] = $k;
      $params['label'] = $v['label'];
      $params['filter'] = $v['filter'];
      $create_result = civicrm_api("OptionValue","create", $params);
      $create_value = array_pop($create_result['values']);
      if($k != 'Formal Title' && $k != 'OpenID') {
        // We don't display Formal Title
        $ids[] = $create_value['value'];
      }
    }

    // In addition, we want to add other options to be checked by default
    $sql = "SELECT value FROM civicrm_option_value WHERE option_group_id = %0 AND name IN ('Email','Phone','IM', 'Website')";
    $query_params = array ( 0 => array($id, 'Integer'));
    $dao = CRM_Core_DAO::executeQuery($sql, $query_params);
    while($dao->fetch()) {
      $ids[] = $dao->value;
    }
    // now ensure the right ones are checked
    pbd_setting('contact_edit_options', $ids, 'CiviCRM Preferences');
  }
}

function pbd_fix_user_dashboard_options() {
  _civicrm_init();
  // now run for user_dashboard_options
  $results = civicrm_api("OptionGroup","get", array ('version' => 3,'name' => 'user_dashboard_options' ));
  if($results['is_error'] == 0) {
    $values = array_pop($results['values']);
    $id = $values['id'];

    $sql = "SELECT value FROM civicrm_option_value WHERE option_group_id = %0";
    $query_params = array ( 0 => array($id, 'Integer'));
    $dao = CRM_Core_DAO::executeQuery($sql, $query_params);
    $ids = array();
    while($dao->fetch()) {
      $ids[] = $dao->value;
    }
    $codeVer = CRM_Utils_System::version();
    if(version_compare($codeVer, '4.7', '<')) {
      // We need a different format for the values if we're using the pre 4.7 method.
      $ids = CRM_Core_DAO::VALUE_SEPARATOR . implode(CRM_Core_DAO::VALUE_SEPARATOR, $ids) . CRM_Core_DAO::VALUE_SEPARATOR;
    }
    pbd_setting('user_dashboard_options', $ids, 'CiviCRM Preferences');
  }
}

function pbd_turn_off_version_check() {
  $codeVer = CRM_Utils_System::version();
  if(version_compare($codeVer, '4.7', '<')) {
    // In 4.7 this is a cron job that can be left disabled.
    pbd_setting('versionCheck', 0);
  }
}

function pbd_turn_off_empowered_by_civicrm_footer() {
  pbd_setting('empoweredBy', 0);
}

function pbd_turn_off_pdf_receipt() {
  // this setting means do not attach pdf receipt...
  pbd_setting('doNotAttachPDFReceipt', 0);
}

function pbd_disable_event_details() {
  _civicrm_init();
  $results = civicrm_api("CustomGroup","get", array ('version' => 3, 'name' => 'event_details'));
  if($results['is_error'] == 0) {
    $values = array_pop($results['values']);
    $id = $values['id'];
    if(!empty($id)) {
      $params = array(
        'id' => $id,
        'is_active' => 0
      );
      $result = civicrm_api3("CustomGroup","update", $params);
    }
  }
}

/**
 * Return the contact id of the user running this script.
 *
 * Some things we create have to have a contact_id for the contact
 * that created them. This function tries to return the most sensible
 * contact id for that purpose.
 */
function pbd_get_my_contact_id() {
  // First see if there's a contact that is related to the Drupal user
  // with UID 1.
  $params = array(
    'uf_id' => 1
  );
  try {
    $result = civicrm_api3('UFMatch', 'get', $params);
  }
  catch (CiviCRM_API3_Exception $e) {
    drush_log(dt("Failed to do UFMatch lookup."), 'error');
    drush_log($e->getMessage(), 'error');
    return FALSE;
  }
  if($result['count'] == 1) {
    $value = array_pop($result['values']);
    return $value['contact_id'];
  }
  // Try to pick the contact_id with the lowest number
  $sql = "SELECT MIN(id) AS min_id FROM civicrm_contact WHERE is_deleted = 0";
  $dao = CRM_Core_DAO::executeQuery($sql);
  $dao->fetch();
  if(!empty($dao->min_id)) return $dao->min_id;
  // Give up. Return 0. Hope for the best.
  drush_log(dt("Failed to find my contact id."), 'error');
  return 0;
}

// designed to be run on install to delete all default tags
function pbd_delete_all_tags() {
  _civicrm_init();
  $sql = "DELETE FROM civicrm_tag";
  CRM_Core_DAO::executeQuery( $sql );
}

// Wrapper for pm_module_list/drush_module_list.
// pm_module_list is drush 5.0. drush_module_list
// is later versions.
function pbd_module_list() {
  if(function_exists('pm_module_list')) {
    return pm_module_list();
  }
  else {
    return drush_module_list();
  }

}

function pbd_disable_modules() {
  $disable = array('update', 'contextual','dashboard','file','image','overlay','rdf','search','shortcut','toolbar','bartik','seven');
  $modules_enabled = pbd_module_list();
  $modules_to_disable = array();
  while(list(,$module) = each($disable)) {
    if(in_array($module, $modules_enabled)) {
      $modules_to_disable[] = $module;
    }
  }
  if(count($modules_to_disable) > 0) {
    drush_log(dt("Disabling unneeded modules."));
    module_disable($modules_to_disable);
  }
}

function pbd_get_voter_info_custom_group_params() {
  $source = array(
    'name' => 'Voter_Info',
    'title' => 'Voter Info',
    'extends' => 'Individual',
    'style' => 'Tab',
    'collapse_display' => '0',
    'help_pre' => '',
    'help_post' => '',
    'weight' => '7',
    'is_active' => '1',
    'is_multiple' => '0',
    'collapse_adv_display' => '0'
  );
  return $source;
}

function pbd_get_voter_info_custom_field_params($custom_group_id,$option_group_ids) {
  $source[] = array(
    'custom_group_id' => $custom_group_id,
    'label' => 'Precinct',
    'data_type' => 'String',
    'html_type' => 'Text',
    'is_required' => '0',
    'is_searchable' => '1',
    'is_search_range' => '0',
    'weight' => '124',
    'is_active' => '1',
    'is_view' => '0',
    'text_length' => '255',
    'date_format' => array(
      '0' => 'M',
      '1' => 'd',
      '2' => 'Y',
    ),
    'note_columns' => '60',
    'note_rows' => '4',
  );

  $source[] = array(
    'custom_group_id' => $custom_group_id,
    'label' => 'State Senate District',
    'data_type' => 'String',
    'html_type' => 'Text',
    'is_required' => '0',
    'is_searchable' => '1',
    'is_search_range' => '0',
    'weight' => '140',
    'is_active' => '1',
    'is_view' => '0',
    'text_length' => '255',
    'note_columns' => '60',
    'note_rows' => '4',
  );
  $source[] = array(
    'custom_group_id' => $custom_group_id,
    'label' => 'City District',
    'data_type' => 'String',
    'html_type' => 'Text',
    'is_required' => '0',
    'is_searchable' => '1',
    'is_search_range' => '0',
    'weight' => '133',
    'is_active' => '1',
    'is_view' => '0',
    'text_length' => '255',
    'date_format' => array(
      '0' => 'M',
      '1' => 'd',
      '2' => 'Y',
    ),
    'note_columns' => '60',
    'note_rows' => '4',
  );
  $source[] = array(
    'custom_group_id' => $custom_group_id,
    'label' => 'School District',
    'data_type' => 'String',
    'html_type' => 'Text',
    'is_required' => '0',
    'is_searchable' => '1',
    'is_search_range' => '0',
    'weight' => '149',
    'is_active' => '1',
    'is_view' => '0',
    'text_length' => '255',
    'note_columns' => '60',
    'note_rows' => '4',
  );
  $source[] = array(
    'custom_group_id' => $custom_group_id,
    'label' => 'County District',
    'data_type' => 'String',
    'html_type' => 'Text',
    'is_required' => '0',
    'is_searchable' => '1',
    'is_search_range' => '0',
    'weight' => '154',
    'is_active' => '1',
    'is_view' => '0',
    'text_length' => '255',
    'note_columns' => '60',
    'note_rows' => '4',
  );

  $source[] = array(
    'custom_group_id' => $custom_group_id,
    'label' => 'Congressional District',
    'data_type' => 'String',
    'html_type' => 'Text',
    'is_required' => '0',
    'is_searchable' => '1',
    'is_search_range' => '0',
    'weight' => '158',
    'is_active' => '1',
    'is_view' => '0',
    'text_length' => '255',
    'date_format' => array(
      '0' => 'M',
      '1' => 'd',
      '2' => 'Y',
    ),
    'note_columns' => '60',
    'note_rows' => '4',
  );
  $source[] = array(
    'custom_group_id' => $custom_group_id,
    'label' => 'Party Registration',
    'data_type' => 'String',
    'html_type' => 'Radio',
    'is_required' => '0',
    'is_searchable' => '1',
    'is_search_range' => '0',
    'weight' => '8',
    'is_active' => '1',
    'is_view' => '0',
    'text_length' => '255',
    'date_format' => array(
      '0' => 'M',
      '1' => 'd',
      '2' => 'Y',
    ),
    'note_columns' => '60',
    'note_rows' => '4',
    'option_group_id' => $option_group_ids['party_registration'],
  );
  $source[] = array(
    'custom_group_id' => $custom_group_id,
    'label' => 'If other party...',
    'data_type' => 'String',
    'html_type' => 'Text',
    'is_required' => '0',
    'is_searchable' => '1',
    'is_search_range' => '0',
    'weight' => '30',
    'is_active' => '1',
    'is_view' => '0',
    'text_length' => '255',
    'date_format' => array(
      '0' => 'M',
      '1' => 'd',
      '2' => 'Y',
    ),
    'note_columns' => '60',
    'note_rows' => '4',
  );
  $source[] = array(
    'custom_group_id' => $custom_group_id,
    'label' => 'County Name',
    'data_type' => 'String',
    'html_type' => 'Select',
    'is_required' => '0',
    'is_searchable' => '1',
    'is_search_range' => '0',
    'weight' => '166',
    'is_active' => '1',
    'is_view' => '0',
    'text_length' => '255',
    'date_format' => array(
      '0' => 'M',
      '1' => 'd',
      '2' => 'Y',
    ),
    'note_columns' => '60',
    'note_rows' => '4',
    'option_group_id' => $option_group_ids['county_name'],
  );
  $source[] = array(
    'custom_group_id' => $custom_group_id,
    'name' => 'Van_ID',
    'label' => 'Van ID',
    'data_type' => 'String',
    'html_type' => 'Text',
    'is_required' => '0',
    'is_searchable' => '1',
    'is_search_range' => '0',
    'weight' => '188',
    'is_active' => '1',
    'is_view' => '0',
    'text_length' => '255',
    'note_columns' => '60',
    'note_rows' => '4',
  );
  $source[] = array(
    'custom_group_id' => $custom_group_id,
    'name' => 'DW_ID',
    'label' => 'DW ID',
    'data_type' => 'String',
    'html_type' => 'Text',
    'is_required' => '0',
    'is_searchable' => '1',
    'is_search_range' => '0',
    'weight' => '186',
    'is_active' => '1',
    'is_view' => '0',
    'text_length' => '255',
    'note_columns' => '60',
    'note_rows' => '4',
  );
  $source[] = array(
    'custom_group_id' => $custom_group_id,
    'name' => 'CountyFileID',
    'label' => 'CountyFileID',
    'data_type' => 'String',
    'html_type' => 'Text',
    'is_required' => '0',
    'is_searchable' => '0',
    'is_search_range' => '0',
    'weight' => '193',
    'is_active' => '1',
    'is_view' => '0',
    'text_length' => '255',
    'note_columns' => '60',
    'note_rows' => '4',
  );
  $source[] = array(
    'custom_group_id' => $custom_group_id,
    'name' => 'PrecinctName',
    'label' => 'PrecinctName',
    'data_type' => 'String',
    'html_type' => 'Text',
    'is_required' => '0',
    'is_searchable' => '1',
    'is_search_range' => '0',
    'weight' => '33',
    'is_active' => '1',
    'is_view' => '0',
    'text_length' => '255',
    'note_columns' => '60',
    'note_rows' => '4',
  );
  $source[] = array(
    'custom_group_id' => $custom_group_id,
    'name' => 'Voter_Registration_Date',
    'label' => 'Voter Registration Date',
    'data_type' => 'Date',
    'html_type' => 'Select Date',
    'is_required' => '0',
    'is_searchable' => '1',
    'is_search_range' => '1',
    'weight' => '171',
    'is_active' => '1',
    'is_view' => '0',
    'text_length' => '255',
    'date_format' => 'mm/dd/yy',
    'note_columns' => '60',
    'note_rows' => '4',
  );
  $source[] = array(
    'custom_group_id' => $custom_group_id,
    'name' => 'State_Voter_File_ID',
    'label' => 'State Voter File ID',
    'data_type' => 'String',
    'html_type' => 'Text',
    'is_required' => '0',
    'is_searchable' => '0',
    'is_search_range' => '0',
    'weight' => '192',
    'is_active' => '1',
    'is_view' => '0',
    'text_length' => '255',
    'note_columns' => '60',
    'note_rows' => '4',
  );
  $source[] = array(
    'custom_group_id' => $custom_group_id,
    'name' => 'State_Assembly_District',
    'label' => 'State Assembly District',
    'data_type' => 'String',
    'html_type' => 'Text',
    'is_required' => '0',
    'is_searchable' => '1',
    'is_search_range' => '0',
    'weight' => '135',
    'is_active' => '1',
    'is_view' => '0',
    'text_length' => '255',
    'note_columns' => '60',
    'note_rows' => '4',
  );
  $source[] = array(
    'custom_group_id' => $custom_group_id,
    'name' => 'Elections_2011',
    'label' => 'Elections 2011',
    'data_type' => 'String',
    'html_type' => 'CheckBox',
    'default_value' => array(
      '0' => 'GY',
    ),
    'is_required' => '0',
    'is_searchable' => '1',
    'is_search_range' => '0',
    'weight' => '172',
    'is_active' => '1',
    'is_view' => '0',
    'text_length' => '255',
    'note_columns' => '60',
    'note_rows' => '4',
    'option_group_id' => $option_group_ids['election'],
  );
  $source[] = array(
    'custom_group_id' => $custom_group_id,
    'name' => 'Elections_2010',
    'label' => 'Elections 2010',
    'data_type' => 'String',
    'html_type' => 'CheckBox',
    'default_value' => array(
      '0' => 'GY',
    ),
    'is_required' => '0',
    'is_searchable' => '1',
    'is_search_range' => '0',
    'weight' => '173',
    'is_active' => '1',
    'is_view' => '0',
    'text_length' => '255',
    'note_columns' => '60',
    'note_rows' => '4',
    'option_group_id' => $option_group_ids['election'],

  );
  $source[] = array(
    'custom_group_id' => $custom_group_id,
    'name' => 'Elections_2009',
    'label' => 'Elections 2009',
    'data_type' => 'String',
    'html_type' => 'CheckBox',
    'default_value' => array(
      '0' => 'GY',
    ),
    'is_required' => '0',
    'is_searchable' => '1',
    'is_search_range' => '0',
    'weight' => '174',
    'is_active' => '1',
    'is_view' => '0',
    'text_length' => '255',
    'note_columns' => '60',
    'note_rows' => '4',
    'option_group_id' => $option_group_ids['election'],

  );
  $source[] = array(
    'custom_group_id' => $custom_group_id,
    'name' => 'Elections_2008',
    'label' => 'Elections 2008',
    'data_type' => 'String',
    'html_type' => 'CheckBox',
    'default_value' => array(
      '0' => 'GY',
    ),
    'is_required' => '0',
    'is_searchable' => '1',
    'is_search_range' => '0',
    'weight' => '176',
    'is_active' => '1',
    'is_view' => '0',
    'text_length' => '255',
    'note_columns' => '60',
    'note_rows' => '4',
    'option_group_id' => $option_group_ids['election'],
  );
  $source[] = array(
    'custom_group_id' => $custom_group_id,
    'name' => 'Elections_2007',
    'label' => 'Elections 2007',
    'data_type' => 'String',
    'html_type' => 'CheckBox',
    'default_value' => array(
      '0' => 'GY',
    ),
    'is_required' => '0',
    'is_searchable' => '1',
    'is_search_range' => '0',
    'weight' => '178',
    'is_active' => '1',
    'is_view' => '0',
    'text_length' => '255',
    'note_columns' => '60',
    'note_rows' => '4',
    'option_group_id' => $option_group_ids['election'],
  );
  $source[] = array(
    'custom_group_id' => $custom_group_id,
    'name' => 'Elections_2006',
    'label' => 'Elections 2006',
    'data_type' => 'String',
    'html_type' => 'CheckBox',
    'default_value' => array(
      '0' => 'GY',
    ),
    'is_required' => '0',
    'is_searchable' => '1',
    'is_search_range' => '0',
    'weight' => '180',
    'is_active' => '1',
    'is_view' => '0',
    'text_length' => '255',
    'note_columns' => '60',
    'note_rows' => '4',
    'option_group_id' => $option_group_ids['election'],
  );
  $source[] = array(
    'custom_group_id' => $custom_group_id,
    'name' => 'Elections_2005',
    'label' => 'Elections 2005',
    'data_type' => 'String',
    'html_type' => 'CheckBox',
    'default_value' => array(
      '0' => 'GY',
    ),
    'is_required' => '0',
    'is_searchable' => '1',
    'is_search_range' => '0',
    'weight' => '182',
    'is_active' => '1',
    'is_view' => '0',
    'text_length' => '255',
    'note_columns' => '60',
    'note_rows' => '4',
    'option_group_id' => $option_group_ids['election'],
  );
  $source[] = array(
    'custom_group_id' => $custom_group_id,
    'name' => 'Date_of_Voter_Data',
    'label' => 'Date of Voter Data',
    'data_type' => 'Date',
    'html_type' => 'Select Date',
    'is_required' => '0',
    'is_searchable' => '1',
    'is_search_range' => '1',
    'weight' => '183',
    'is_active' => '1',
    'is_view' => '0',
    'text_length' => '255',
    'date_format' => 'mm/dd/yy',
    'note_columns' => '60',
    'note_rows' => '4',
  );
  return $source;
}

function pbd_get_election_option_group_params() {
  $source = array(
    'name' => 'election',
    'title' => 'Election',
    'is_reserved' => '1',
    'is_active' => '1'
  );
  return $source;
}

function pbd_get_voter_history_option_group_params() {
  $source = array(
    'name' => 'voter_history_values',
    'title' => 'Voter History Values',
    'is_reserved' => '1',
    'is_active' => '1'
  );
  return $source;
}

function pbd_get_county_name_option_group_params() {
  $source = array(
    'name' => 'county_name_options',
    'title' => 'County Name Options',
    'is_reserved' => '1',
    'is_active' => '1'
  );
  return $source;
}

function pbd_get_party_registration_option_group_params() {
  $source = array(
    'name' => 'party_registration_options',
    'title' => 'Party Registration Options',
    'is_reserved' => '1',
    'is_active' => '1'
  );
  return $source;
}

function pbd_get_party_registration_option_value_params($option_group_id) {
  $source[] = array(
    'option_group_id' => $option_group_id,
    'label' => 'Democrat',
    'value' => 'D',
    'name' => 'Democrat',
    'is_default' => '0',
    'weight' => '1',
    'is_optgroup' => '0',
    'is_reserved' => '0',
    'is_active' => '1'
  );
  $source[] = array(
    'option_group_id' => $option_group_id,
    'label' => 'Green',
    'value' => 'G',
    'name' => 'Green',
    'is_default' => '0',
    'weight' => '3',
    'is_optgroup' => '0',
    'is_reserved' => '0',
    'is_active' => '1'
  );
  $source[] = array(
    'option_group_id' => $option_group_id,
    'label' => 'Independent',
    'value' => 'I',
    'name' => 'Independent',
    'is_default' => '0',
    'weight' => '4',
    'is_optgroup' => '0',
    'is_reserved' => '0',
    'is_active' => '1'
  );
  $source[] = array(
    'option_group_id' => $option_group_id,
    'label' => 'Libertarian',
    'value' => 'L',
    'name' => 'Libertarian',
    'is_default' => '0',
    'weight' => '5',
    'is_optgroup' => '0',
    'is_reserved' => '0',
    'is_active' => '1'
  );
  $source[] = array(
    'option_group_id' => $option_group_id,
    'label' => 'Other',
    'value' => 'O',
    'name' => 'Other',
    'is_default' => '0',
    'weight' => '7',
    'is_optgroup' => '0',
    'is_reserved' => '0',
    'is_active' => '1'
  );
  $source[] = array(
    'option_group_id' => $option_group_id,
    'label' => 'Republican',
    'value' => 'R',
    'name' => 'Republican',
    'is_default' => '0',
    'weight' => '2',
    'is_optgroup' => '0',
    'is_reserved' => '0',
    'is_active' => '1'
  );
  $source[] = array(
    'option_group_id' => $option_group_id,
    'label' => 'Undeclared',
    'value' => 'U',
    'name' => 'Undeclared',
    'is_default' => '0',
    'weight' => '6',
    'is_optgroup' => '0',
    'is_reserved' => '0',
    'is_active' => '1'
  );
  return $source;
}

function pbd_get_election_option_value_params($option_group_id) {
  $source[] = array(
    'option_group_id' => $option_group_id,
    'label' => 'G - N',
    'value' => 'GN',
    'name' => 'G__N',
    'is_default' => '0',
    'weight' => '2',
    'is_optgroup' => '0',
    'is_reserved' => '0',
    'is_active' => '1'
  );
  $source[] = array(
    'option_group_id' => $option_group_id,
    'label' => 'G - Y',
    'value' => 'GY',
    'name' => 'G__Y',
    'is_default' => '0',
    'weight' => '1',
    'is_optgroup' => '0',
    'is_reserved' => '0',
    'is_active' => '1'
  );
  $source[] = array(
    'option_group_id' => $option_group_id,
    'label' => 'P - N',
    'value' => 'PN',
    'name' => 'P__N',
    'is_default' => '0',
    'weight' => '4',
    'is_optgroup' => '0',
    'is_reserved' => '0',
    'is_active' => '1'
  );
  $source[] = array(
    'option_group_id' => $option_group_id,
    'label' => 'P - Y',
    'value' => 'PY',
    'name' => 'P__Y',
    'is_default' => '0',
    'weight' => '3',
    'is_optgroup' => '0',
    'is_reserved' => '0',
    'is_active' => '1'
  );
  $source[] = array(
    'option_group_id' => $option_group_id,
    'label' => 'PP - N',
    'value' => 'PPN',
    'name' => 'PP__N',
    'is_default' => '0',
    'weight' => '6',
    'is_optgroup' => '0',
    'is_reserved' => '0',
    'is_active' => '1'
  );
  $source[] = array(
    'option_group_id' => $option_group_id,
    'label' => 'PP - Y',
    'value' => 'PPY',
    'name' => 'PP__Y',
    'is_default' => '0',
    'weight' => '5',
    'is_optgroup' => '0',
    'is_reserved' => '0',
    'is_active' => '1'
  );
  $source[] = array(
    'option_group_id' => $option_group_id,
    'label' => 'S - N',
    'value' => 'SN',
    'name' => 'S__N',
    'is_default' => '0',
    'weight' => '8',
    'is_optgroup' => '0',
    'is_reserved' => '0',
    'is_active' => '1'
  );
  $source[] = array(
    'option_group_id' => $option_group_id,
    'label' => 'S - Y',
    'value' => 'SY',
    'name' => 'S__Y',
    'is_default' => '0',
    'weight' => '7',
    'is_optgroup' => '0',
    'is_reserved' => '0',
    'is_active' => '1'
  );
  return $source;
}

function pbd_setup_option_group($group_params) {

  // ensure the OptionGroup exists, if not creaate it
  $option_group = pbd_return_entity('OptionGroup', $group_params);
  $option_group_id = $option_group['id'];
  return $option_group_id;
}

function pbd_setup_option_values($group_id, $value_params) {
  // check if any OptionValues exist for this option group
  $values = pbd_get_option_values($group_id);

  // only create if none exist - this allows each group to customize them if they want
  if(count($values) == 0) {
    // create default options
    reset($value_params);
    while(list($k,$v) = each($value_params)) {
      pbd_return_entity('OptionValue', $v);
    }
  }
}



function pbd_get_custom_field_set($name) {
  _civicrm_init();
  require_once ('api/api.php');
  $params = array(
    'version' => 3,
    'name' => $name,
  );

  $results = civicrm_api('CustomGroup', 'get', $params);
  if(count($results['values']) == 0) return array();
  $voter_info_custom_group = array_pop($results['values']);
  $custom_group_id = $voter_info_custom_group['id'];

  $params = array(
    'version' => 3,
    'custom_group_id' => $custom_group_id,
  );
  $results = civicrm_api('CustomField', 'get', $params);
  return $results['values'];
}

function pbd_get_option_values($id) {
  _civicrm_init();
  require_once ('api/api.php');
  $params = array(
    'version' => 3,
    'option_group_id' => $id,
  );
  $results = civicrm_api('OptionValue', 'get', $params);
  return $results['values'];
}

/**
 * Either fetch the matching entity or create it and
 * then return it.
 */
function pbd_return_entity($entity, $params, $key = 'name') {
  _civicrm_init();
  require_once ('api/api.php');

  $search_params[$key] = $params[$key];
  $params['version'] = $search_params['version'] = 3;
  $results = civicrm_api($entity, 'get', $search_params);
  if(count($results['values']) == 0) {
    $results = civicrm_api($entity, 'create', $params);
  }
  if($results['is_error'] != 0) {
    drush_print(dt("Error! ") . $results['error_message'] );
    return array();
  }
  return array_pop($results['values']);
}

/**
 * Setup default themekey rules.
 */
function pbd_configure_themekey() {
  $existing = db_query("SELECT status FROM {system} WHERE type = 'module' AND name = 'themekey'")->fetchObject();
  // Only configure if themekey is enabled.
  if($existing->status == 1) {
    // We have default settings for the major changes we often use. By default, we set
    // the theme to the default theme, making it easy to change once we have a new
    // custom theme.
    $settings = array(
      '(civicrm/profile/edit|civicrm/profile/view|civicrm/profile/create)',
      '(civicrm/mailing/subscribe|civicrm/mailing/unsubscribe|civicrm/mailing/optout|civicrm/mailing/resubscribe|civicrm/mailing/confirm)',
      '(civicrm/contribute/transact|civicrm/pcp|civicrm/contribute/campaign|civicrm/contribute/unsubscribe|civicrm/contribute/updatebilling|civicrm/contribute/updaterecur)',
      '(civicrm/event/info|civicrm/event/register|civicrm/event/confirm|civicrm/mailing/forward|civicrm/event/ical|civicrm/event/view_cart|event/cart_checkout)',
    );
    $weight_query = db_query("SELECT MAX(weight) AS weight FROM {themekey_properties}")->fetchObject();
    $weight = empty($weight_query->weight) ? 1 : $weight_query->weight;
    foreach($settings as $value) {
      $status = db_query("SELECT id FROM {themekey_properties} WHERE value = :value",
        array(':value' => $value))->fetchObject();
      if(empty($status->id)) {
        // Insert it
        db_insert("themekey_properties")
          ->fields(array(
            'property' => 'drupal:get_q',
            'operator' => '~',
            'value' => $value,
            'weight' => $weight,
            'theme' => 'default',
            'enabled' => 1,
            'wildcards' => serialize(array()),
            'module' => 'themekey'
          ))->execute();
      }
    }
  }
}

/**
 * Enable all third party modules that we use.
 */
function pbd_enable_modules() {
  $modules_wanted = array(
    'themekey', 'opb',
    'civicrm_cicero', 'ctools', 'views', 'views_ui',
    'webform_civicrm', 'file',
    'webform','options_element',
    'libraries', 'autologout', 'themekey_civicrm',
    'themekey_ui', 'jquery_update', 'path'
  );

  drush_log(dt("Checking which modules are already enabled."));

  $modules_already_enabled = pbd_module_list();
  $modules_to_enable = array();
  while(list(,$module) = each($modules_wanted)) {
    if(!in_array($module, $modules_already_enabled)) {
      $modules_to_enable[] = $module;
    }
  }
  if(count($modules_to_enable) > 0) {
    drush_log(dt("Enabling Drupal modules."));
    module_enable($modules_to_enable);
  }
  else {
    drush_log(dt("No modules to enable."));
  }

}

/**
 * See: https://app.liquidplanner.com/space/48716/projects/show/7843451P
 *
 * On creation of a new site, ensure there is no default header to avoid
 * people accidentally sending embarrassing messages with a default
 * header set.
 *
 **/
function pbd_unset_default_header() {
  // Make the pb email footer the default
  _civicrm_init();
  $sql = "UPDATE civicrm_mailing_component SET is_default = 0 WHERE component_type = 'Header'";
  CRM_Core_DAO::executeQuery($sql);
}

function pbd_ensure_pb_email_footer_exists($settings) {
  require_once 'CRM/Mailing/BAO/Component.php';
  $search_params = array(
    'name' => $settings['name'],
  );
  $component = CRM_Mailing_BAO_Component::retrieve($search_params, $defaults);
  if(empty($component)) {
    // We need to create it
    $ids = array();
    CRM_Mailing_BAO_Component::add($settings, $ids);
    $component = CRM_Mailing_BAO_Component::retrieve($search_params, $defaults);
  }
  return $component->id;
}

function pbd_add_include_exclude_search_menu() {
  _civicrm_init();
  $label = 'Include/Exclude Search';

  require_once 'CRM/Core/BAO/Navigation.php';
  $menus = CRM_Core_BAO_Navigation::getNavigationList();
  $exists_array = preg_grep("#$label#", $menus);
  if(count($exists_array)) {
    drush_log(dt("Include/Exclude Menu already added."), 'notice');
    return;
  }
  // Get the value for the custom search - we need it for the URL
  $params = array('label' => 'CRM_Contact_Form_Search_Custom_Group');
  try {
    $value = civicrm_api3('OptionValue', 'getsingle', $params);
  }
  catch(CiviCRM_API3_Exception $e) {
    drush_log(dt("Failed to find Inclue/Exclude value."), 'error');
    drush_log($e->getMessage(), 'error');
    return;
  }
  $url = 'civicrm/contact/search/custom?csid=' . $value['value'] . '&reset=1';
  $menu_array = preg_grep('/^Search/', $menus);
  $name = array_pop($menu_array);
  $keys = array_keys($menus, $name);
  if (count($keys) != 1) {
    drush_print(dt("Failed to add Include/Exclude search menu item."), 'error');
    return;
  }
  $id = array_pop($keys);
  $params = array(
    'label' => $label,
    'url' => $url,
    'permission' => array(
      0 => 'access CiviCRM',
    ),
    'permission_operator' => 'AND',
    'is_active' => 1,
    'parent_id' => $id,
  );
  CRM_Core_BAO_Navigation::add($params);
}

function pbd_add_image_upload_menu() {
  _civicrm_init();
  $label = 'Uploaded Images';
  $url = 'sites/all/modules/civicrm/packages/kcfinder/browse.php?cms=civicrm&type=images';
  require_once 'CRM/Core/BAO/Navigation.php';
  $menus = CRM_Core_BAO_Navigation::getNavigationList();
  $exists_array = preg_grep("/$label/", $menus);
  if(count($exists_array)) {
    return;
  }
  $menu_array = preg_grep('/Customize Data and Screens/', $menus);
  $name = array_pop($menu_array);
  $keys = array_keys($menus, $name);
  if (count($keys) != 1) {
    drush_print(dt("Failed to add image upload menu item."));
    return;
  }
  $id = array_pop($keys);
  $params = array(
    'label' => $label,
    'url' => $url,
    'permission' => array(
      0 => 'access CiviCRM',
    ),
    'permission_operator' => 'AND',
    'is_active' => 1,
    'parent_id' => $id,
  );
  CRM_Core_BAO_Navigation::add($params);
}

/**
 * Ensure exported dates don't have ordinals because it confuses
 * MS Excel
 **/
function pbd_set_default_dateformat_full() {
  _civicrm_init();
  pbd_setting('dateformatFull', '%B %E, %Y');
}

/**
 * Ensure admin theme is set to default so all sites have consistent
 * look and feel in Drupal back-end and so all blocks consistently appear
 **/
function pbd_set_admin_theme() {
  variable_set('admin_theme', '0');
  variable_set('node_admin_theme', 0);
}

/**
 * We don't want webforms showing up on front page
 **/

function pbd_disable_webform_promote_to_front() {
  $var = variable_get('node_options_webform');
  if($var) {
    $id = array_search('promote', $var);
    if($id !== FALSE) {
      unset($var[$id]);
      variable_set('node_options_webform', $var);
    }
  }
}

/**
 * Ensure proper jquery version is selected.
 */
function pbd_configure_jquery_update() {
  if(module_exists('jquery_update')) {
    variable_set('jquery_update_compression_type', 'min');
    variable_set('jquery_update_jquery_admin_version', '');
    variable_set('jquery_update_jquery_cdn', 'none');
    variable_set('jquery_update_jquery_version', '1.10');
  }
}

/**
 * Confgure the autologout module default time.
 *
 * This is run once on install and can be changed via the UI. 
 **/
function pbd_configure_autologout() {
  $autologout_timeout = 7200;
  $autologout_redirect_url = 'civicrm';

  variable_set('autologout_timeout', $autologout_timeout);
  variable_set('autologout_redirect_url', $autologout_redirect_url);
}

/**
 * Helper function. Return custom field id for the first part
 * of the custom field name.
 **/
function pbd_id_for_custom_field_name($name) {
  _civicrm_init();
  // Get the id of the ward field, which may be different
  // on each site.
  $sql = "SELECT id FROM civicrm_custom_field WHERE column_name LIKE %1";
  $params = array(
    1 => array($name . '%', 'String')
  );
  $dao = CRM_Core_DAO::executeQuery($sql, $params);
  $dao->fetch();
  if(empty($dao->id)) {
    return FALSE;
  }
  return $dao->id;
}

/**
 * Used to safely delete un-used custom fields.
 **/
function pbd_remove_custom_field_if_unused($id) {
  if(pbd_custom_field_is_unused($id)) {
    $params = array('id' => $id, 'version' => 3);
    $results = civicrm_api('CustomField','delete',$params);
    if($results['is_error'] == 0) {
      return TRUE;
    }
    else {
      drush_print(dt($results['error_message']));
    }
  }
  return FALSE;
}

/**
 * Returns FALSE (and emits error via drush_print)
 * if the custom field is used in a profile or contains
 * data. Otherwise, returns TRUE.
 **/
function pbd_custom_field_is_unused($id) {

  if(!pbd_column_is_empty($id)) {
    drush_print("Found values in the custom table.");
    return FALSE;
  }
  if(!pbd_custom_field_not_used_in_profile($id)) {
    drush_print("Field is used in a profile.");
    return FALSE;
  }
  return TRUE;
}

/**
 * Helper function - returns column name
 * and table name for custom field i.
 **/
function pbd_table_and_column_name_for_custom_field_id($id) {
  $sql = "SELECT column_name, table_name FROM civicrm_custom_field cf ".
   "JOIN civicrm_custom_group cg ON cf.custom_group_id = cg.id  WHERE cf.id = %1";
  $params = array(
    1 => array($id, 'Integer')
  );
  $dao = CRM_Core_DAO::executeQuery($sql, $params);
  $dao->fetch();
  if(empty($dao->column_name)) {
    return FALSE;
  }
  return array($dao->column_name => $dao->table_name);
}

/**
 * Helper function. Returns TRUE if the field passed
 * contains no data, so is safe to delete.
 **/
function pbd_column_is_empty($id) {
  $field_details = pbd_table_and_column_name_for_custom_field_id($id);
  if(FALSE === $field_details) {
    drush_print("Can't find custom field with that id.");
    return FALSE;
  }
  list($column_name, $table_name) = each($field_details);
  // Ensure there is no data
  $sql = "SELECT COUNT(*) AS count FROM `$table_name` WHERE ".
    "`$column_name` IS NOT NULL AND `$column_name` != ''";
  $dao = CRM_Core_DAO::executeQuery($sql);
  $dao->fetch();
  if($dao->count != 0) {
    return FALSE;
  }
  return TRUE;
}

/**
 * Helper function. Returns TRUE if the field is not being
 * used in a profile, or FALSE if it is.
 **/
function pbd_custom_field_not_used_in_profile($id) {
  $sql = "SELECT COUNT(*) AS count FROM civicrm_uf_field WHERE field_name = %1";
  $params = array(
    1 => array('custom_' . $id, 'String')
  );
  $dao = CRM_Core_DAO::executeQuery($sql, $params);
  $dao->fetch();
  if($dao->count == 0) return TRUE;
  return FALSE;
}

/**
 * Configure default civicrm cicero mappings
 **/
function pbd_configure_default_cicero_mappings() {
  // LOCAL => city_district, NATIONAL_LOWER => congressional_district,
  // STATE_LOWER => state_assembly_district, STATE_UPPER => state_senate_district
  $local = pbd_id_for_custom_field_name('city_district');
  $national_lower = pbd_id_for_custom_field_name('federal_district');
  $state_lower = pbd_id_for_custom_field_name('state_assembly_district');
  $state_upper = pbd_id_for_custom_field_name('state_senate_district');

  $map = array();
  if($local) $map['LOCAL'] = 'custom_' . $local;
  if($national_lower) $map['NATIONAL_LOWER'] = 'custom_' . $national_lower;
  if($state_lower) $map['STATE_LOWER'] = 'custom_' . $state_lower;
  if($state_upper) $map['STATE_UPPER'] = 'custom_' . $state_upper;
  variable_set('civicrm_cicero_contact_field_map', $map);
}


/**
 * Civicrm 4.3 upgrade adds civicrm news, which pulls in blog posts
 * from the CiviCRM web site (using http) and then displays the posts
 * on your dashboard. This is a security problem. This function
 * removes it from your dashboard (but it still is available via
 * /civicrm/dashboard/blog). In addition, 4.6.something added a
 * Getting Started one.
 *
 **/
function pbd_ensure_civicrm_news_removed_from_dashboard() {
  _civicrm_init();

  // Turn off CiviCRM community messages by setting the URL to empty. Note:
  // if we set it to '' - then CiviCRM resets it to the default.
  pbd_setting('communityMessagesUrl', ' ');

  // Now remove the news from the Dashboard.
  $sql = "SELECT id FROM civicrm_dashboard WHERE url LIKE 'civicrm/dashlet/blog%' OR url LIKE 'civicrm/dashlet/getting-started%'";
  $dao = CRM_Core_DAO::executeQuery($sql);
  // It's already been deleted, so return
  if($dao->N == 0) {
    return;
  }
  while($dao->fetch()) {
    // first delete all instances of this dashboard item
    $sql = "DELETE FROM civicrm_dashboard_contact WHERE dashboard_id = %0";
    $params = array(0 => array($dao->id, 'Integer'));
    CRM_Core_DAO::executeQuery($sql, $params);

    // Now delete the dashboard item so nobody will try to add it in the future
    $sql = "DELETE FROM civicrm_dashboard WHERE id = %0";
    $params = array(0 => array($dao->id, 'Integer'));
    CRM_Core_DAO::executeQuery($sql, $params);
  }
}

/**
 * Configure google recaptcha
 *
 * If values are present in the environment, add them to CiviCRM.
 * Sites should evaluate whether they want to invite Google into
 * their CRM by enabling it for a profile.
 **/
function pbd_configure_google_recaptcha() {
  $secret = getenv('LONG_GOOGLE_RECAPTCHA_SECRET_KEY');
  $site = getenv('LONG_GOOGLE_RECAPTCHA_SITE_KEY');
  if($secret) {
    drush_log(dt("Configuring Google Captcha"), 'notice');
    pbd_setting('recaptchaPrivateKey', $secret);
  }
  if($site) {
    pbd_setting('recaptchaPublicKey', $site);
  }
}

/**
 * Configure google geo/map api
 *
 */
function pbd_configure_google_geo_map() {
  $key = getenv('LONG_GOOGLE_MAP_GEO_API_KEY');
  $live = getenv('LONG_LIVE');
  drush_log(dt("Configuring Google Geo/Mapping key"), 'notice');
  pbd_setting('geoAPIKey', $key);
  pbd_setting('mapAPIKey', $key);

}

/**
 * Add settings properly based on version
 *
 * Rip out everything except the api call after we upgrade to 4.7
 *
 * 4.7 introduced a new method for adding settings.
 * Previously, we used either:
 *   CRM_Core_BAO_ConfigSetting::add($config);
 * or
 *   CRM_Core_BAO_Setting::setItem($value, $group, $key);
 *
 * If version is earlier than 4.7 and $group is specified
 * then use the setItem method, otherwise use the add method.
 */
function pbd_setting($key, $value, $group = NULL) {
  $codeVer = CRM_Utils_System::version();
  $params = array($key => $value);
  if(version_compare($codeVer, '4.7', '>=')) {
    civicrm_api3('setting', 'create', $params);
  }
  else {
    if($group) {
      require_once "CRM/Core/BAO/Setting.php";
      if(is_array($value)) {
        // arrays have to be converted to strings first.
        $value = CRM_Core_DAO::VALUE_SEPARATOR . implode(CRM_Core_DAO::VALUE_SEPARATOR, $value) . CRM_Core_DAO::VALUE_SEPARATOR;
      }
      CRM_Core_BAO_Setting::setItem($value, $group, $key);
    }
    else {
      require_once "CRM/Core/BAO/ConfigSetting.php";
      CRM_Core_BAO_ConfigSetting::add($params);
    }
  }
}

/**
 * On a per group basis, set deterministic refresh mode.
 *
 * Smart Group Refresh mode is set to opportunistic by default. That means
 * the smart group cache is flushed whenever a contact changes to ensure
 * it is always up to date. That can cause some locking though. So we 
 * can optionally set it to deterministic which means it is flushed
 * once per cron job instead. If you add a group to this list you
 * *must* also:
 *  * create the cron job via the web UI (Job.group_cache_flush). 
 *  * In Search Preferences, set cache time to 15 minutes
 * Otherwise, it will never get flushed.
 */
function pbd_set_smart_group_refresh_mode() {
  $sites = array('texasenvironment', 'isaiah', 'workingwa', 'southernvision');
  if(in_array(ptp_get_subdomain(), $sites)) {
    $params = array('smart_group_cache_refresh_mode' => 'deterministic');
    drush_log("Setting smart group cache to deterministic.", 'info');
    civicrm_api3('Setting', 'create', $params);
  }
}

/**
 * Ensure we limit list of state provinces to the relevant countries.
 */
function pbd_set_state_province_limit() {
  $key = 'provinceLimit';
  $results = civicrm_api3('setting', 'get', array());
  if(empty($results['values'][$key])) {
    // Set to the US
    $params[$key] = 1228;
    civicrm_api3('setting', 'create', $params);
  }
}

/**
 * Ensure extra fonts are available for certain multi-lingual sites.
 */
function pbd_configure_additional_fonts() {
  $fonts = array(
    "batang" => "Batang",
    "dotum" => "Dotum",
    "hline" => "Hline",
    "gulim" => "Gulim",
  );
  pbd_setting('additional_fonts', $fonts, "CiviCRM Preferences");
}

/**
 * Disable external extension repos
 *
 * We don't want users to be able to install any extension,
 * just the ones we provide.
 **/
function pbd_disable_external_extension_repository() {
  pbd_setting('ext_repo_url', FALSE, "Extension Preferences");
}

/**
 * Enable remote_profile_submissions by default.
 *
 * Starting in 4.7 this is disabled. Most groups want a signup
 * form on their home page.
 */
function pbd_enable_remote_profile_submissions() {
  pbd_setting('remote_profile_submissions', TRUE);
}


/**
 * Set our own dedupe rules.
 */
function pbd_dedupe_rules() {
  // This may need to be redone. See: https://lab.civicrm.org/dev/core/issues/387
  
  // We want the default individual unsupervised rule to be first name, last name 
  // and email (e.g. what the default supervised rule is). We want the default
  // unsupervised rule (email only) to be a general rule. 
  $sql = "UPDATE civicrm_dedupe_rule_group SET used = 'General' WHERE name = 'IndividualUnsupervised'";
  CRM_Core_DAO::executeQuery($sql);
  $sql = "UPDATE civicrm_dedupe_rule_group SET used = 'Unsupervised' WHERE name = 'IndividualSupervised'";
  CRM_Core_DAO::executeQuery($sql);

  // We also want the individual supervised rule to be just first and last name.
  // Create a new rule group if it doesn't already exist.

  $sql = "SELECT id FROM civicrm_dedupe_rule_group WHERE name = 'FirstAndLastName'";
  $dao = CRM_Core_DAO::executeQuery($sql);
  if ($dao->N == 0) {
    $sql = "INSERT INTO civicrm_dedupe_rule_group SET 
      contact_type = 'Individual',
      threshold = '2',
      used = 'Supervised', 
      name = 'FirstAndLastName',
      title = 'First and Last Name'";
    CRM_Core_DAO::executeQuery($sql);

    $sql = "SELECT id FROM civicrm_dedupe_rule_group WHERE name = 'FirstAndLastName'";
    $dao = CRM_Core_DAO::executeQuery($sql);
    $dao->fetch();
    
    $sql = "INSERT INTO civicrm_dedupe_rule SET
      dedupe_rule_group_id = %0,
      rule_table = 'civicrm_contact',
      rule_field = 'first_name',
      rule_length = NULL,
      rule_weight = 1";
    $params = array(0 => array($dao->id, 'Integer'));
    CRM_Core_DAO::executeQuery($sql, $params); 
    $sql = "INSERT INTO civicrm_dedupe_rule SET
      dedupe_rule_group_id = %0,
      rule_table = 'civicrm_contact',
      rule_field = 'last_name',
      rule_length = NULL,
      rule_weight = 1";
    CRM_Core_DAO::executeQuery($sql, $params); 

  }
  // And lastly, we want to fix the organization rules.
  // By default, both supervised and unsupervised are both called "Name and Email"
  // and both are actually Name OR Email. We want unsupervised to be
  // both name and email and supervised to be just name.
  $sql = "UPDATE civicrm_dedupe_rule_group SET threshold = '20' WHERE name = 'OrganizationUnsupervised'";
  CRM_Core_DAO::executeQuery($sql);

  $sql = "SELECT id FROM civicrm_dedupe_rule_group WHERE name = 'OrganizationSupervised'";
  $dao = CRM_Core_DAO::executeQuery($sql);
  $dao->fetch();

  $sql = "DELETE FROM civicrm_dedupe_rule WHERE dedupe_rule_group_id = %0 AND rule_field = 'email'";
  CRM_Core_DAO::executeQuery($sql, array(0 => array($dao->id, 'Integer')));

  $sql = "UPDATE civicrm_dedupe_rule_group SET title = 'Name' WHERE id = %0";
  CRM_Core_DAO::executeQuery($sql, array(0 => array($dao->id, 'Integer')));

}
?>
