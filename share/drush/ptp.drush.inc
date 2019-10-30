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
  $items['ptp-get-site-info'] =
    array(
      'description' => 'Given a powerbase URL, return information about it taken from the PTP database..',
      'options' => array(
        'url' => 'The full PowerBase URL of the site.',
        'info' => 'The info you want: name, email, phone, state, address, zip, state'
      ),
  );
  
  $items['ptp-configure'] =
    array('description' => "Ensure all PTP general settings are properly configured."
  );
  $items['ptp-check-sumfields'] =
    array(
      'description' => "Report if any summary fields contribution triggers are missing.",
      'options' => array(
        'fix' => "Pass --fix if you want problem sites to be auto-fixed."
      ),
    );
  $items['ptp-update-report'] =
    array('description' => "Print out report of modules and themes that need to be updated."
  );
  $items['ptp-fix-missing'] =
    array('description' => "Delete modules and themes that were installed but no longer are present."
  );
  $items['ptp-import-options'] =
    array(
      'description' => "Import options pass via file.",
      'options' => array(
        'location' => dt("The location/address of the CSV file to import."),
        'action' => dt("Either replace, add or new - depending on whether options should replace or add to an existing set or create a new one."),
        'name' => dt("The name of the option group - this is the key that will be used to identify existing option groups."),
        'title' => dt("The friendly name - only needed if action is new.")
      )
    );
  $items['ptp-reset-drupal-email-address'] = 
    array(
      'description' => dt("If the drupal email address is set to a progressivetech.org address, reset it to the CiviCRM default from address.")
    );
  $items['ptp-set-outbound-mail-settings'] = 
    array(
      'description' => dt("Temporary for re-setting to bulk.mayfirst.org for upgade purposes."),
      'options' => array(
        'host' => dt("The host to set it to. Defaults to bulk.ourpowerbase.net")
      ),
    );
  $items['ptp-recreate-multilingual-view'] =
    array(
      'description' => dt("Re-create a multilingual view if the base tables change."),
      'options' => array(
        'locale' => dt("Enter locale, e.g. eo_US or es_MX."),
        'table' => dt("Enter the table, e.g. civicrm_price_field_value")
      )
    );
  $items['ptp-csv'] = array(
    'description' => 'Convert all tables to csv files',
    'options' => array(
      'table' => dt("Optionally, specify the table you want.")
    )
  );
  $items['ptp-is-primary-fix'] = array(
    'description' => 'Find all contacts with one email that is not set to is_primary and set it to is_primary'
  );
  $items['ptp-parse-addresses'] = array(
    'description' => 'Analyze addresses that are not parsing properly.'
  );
  $items['ptp-checksum'] = array(
    'description' => 'validate a checksum',
    'options' => array(
      'checksum' => dt("Provide the checksum."),
      'id' => dt("The contact id to check"),
    ),
  );
  $items['ptp-authorize-to-stripe'] = array(
    'description' => 'move an authorize recurring contribution to stripe',
    'options' => array(
      'id' => dt("The contact id of the person to move"),
      'sub' => dt("The Stripe subscription id we should expect to use from now on"),
      'authid' => dt("Optionally, provide the payment processor id number for authorize (if you have more then one)"),
      'stripeid' => dt("Optionally, provide the payment processor id number for stripe"),
    )
  );
  return $items;
}


function drush_ptp_get_site_info() {
  _civicrm_init();
  $url = drush_get_option('url', NULL);
  $info = drush_get_option('info', NULL);
  if (empty($url)) {
    drush_Log("Please pass the Powerbase url.", "error");
    return FALSE;
  }

  if ($info == 'name') {
    $return = 'organization_name';
  }
  elseif ($info == 'email') {
    $return = 'email';
  }
  elseif ($info == 'state') {
    $return = 'state_province';
  }
  elseif ($info == 'phone') {
    $return = 'phone';
  }
  elseif ($info == 'address') {
    $return = 'street_address';
  }
  elseif ($info == 'city') {
    $return = 'city';
  }
  elseif ($info == 'zip') {
    $return = 'postal_code';
  }
  else {
    drush_Log("I can only return a name, state, phone, address, city, zip or an email.", "error");
    return FALSE;
  }
  $result = civicrm_api3('Contact', 'get', array(
    'sequential' => 1,
    'return' => array($return),
    'custom_335' => $url,
  ));
  if ($result['count'] == 1) {
    $values = array_pop($result['values']);
    $value = $values[$return];
    # We use the results in a bash command so we have to strip
    # ', ", (, ) to avoid bash errors.
    drush_print(str_replace(array('"', "'", "(", ")"), "", $value));
  }
}

function drush_ptp_checksum() {
  _civicrm_init();
  $checksum = drush_get_option('checksum', NULL);
  $contact_id = drush_get_option('id', NULL);
  drush_log("Checking contact id: $contact_id and checksum: $checksum");
  if (CRM_Contact_BAO_Contact_Utils::validChecksum($contact_id, $checksum)) {
    drush_log("Yup, it passes.", "ok");
  }
  else {
    drush_Log("Broken checksum.", "error");
  }

}

function drush_ptp_parse_addresses() {
  $sql = "SELECT ca.id, contact_id, street_address, street_number, street_name, street_number_suffix FROM civicrm_address ca JOIN civicrm_contact c ON ca.contact_id = c.id WHERE
    is_deleted = 0";
  $dao = CRM_Core_DAO::executeQuery($sql);
  $count = 0;
  while($dao->fetch()) {
    $street_name = trim($dao->street_name);
    $street_number = trim($dao->street_number);
    $street_number_suffix = trim($dao->street_number_suffix, ' ');
    $street_address = preg_replace('/  +/', ' ', $dao->street_address);
    if (empty($dao->street_address)) {
      continue;
    }
    if (!preg_match("|" . preg_quote($street_number) . " ?(" . preg_quote($street_number_suffix) . ")? ?" . preg_quote($street_name) . "|i", $street_address)) {
      $count++;
      $msg = dt(
        "Error: @id / @contact_id - @street_number @street_number_suffix @street_name doesn't match @street_address",
        array(
          '@id' => $dao->id,
          '@contact_id' => $dao->contact_id,
          '@street_number' => $street_number,
          '@street_name' => $street_name,
          '@street_address' => $street_address,
          '@street_number_suffix' => $street_number_suffix
        )
      );
      drush_log($msg, 'error');
    }
  }
  drush_log(dt("Count of badly parsed address: @count", array('@count' => $count)), 'ok');
}

// See https://support.ourpowerbase.net/ticket/2408
function drush_ptp_is_primary_fix() {
  _civicrm_init();
  $tables = array('civicrm_email', 'civicrm_phone', 'civicrm_address');
  foreach($tables as $table) {
    $count = 0;
    // Find contacts with just one email address that is NOT set to
    // is_primary and return their civicrm_email id so they can be
    // updated. I am not sure how these records are being created
    // but they cause a lot of user confusion since non primary
    // email addresses don't show up in searches.
    $sql = "SELECT id FROM $table GROUP BY contact_id HAVING COUNT(id) = 1 AND MAX(is_primary) = 0";
    $dao = CRM_Core_DAO::executeQuery($sql);
    while($dao->fetch()) {
      $count++;
      $sql = "UPDATE $table SET is_primary = 1 WHERE id = %0";
      CRM_Core_DAO::executeQuery($sql, array(0 => array($dao->id, 'Integer')));
    }
    drush_log(dt("Updated table @table, @count records changed.", 
      array('@table' => $table, '@count' => $count)));
  }
}

// This is stupid. Anyone who requests this deserves what they get.
function drush_ptp_csv() {
  _civicrm_init();
  // Which tables should we export?
  $tables = array();

  $table = drush_get_option('table', NULL);
  if ($table) {
    // Just one table, the one specified by the user.
    $tables = array($table);
  }
  else {
    // All CiviCRM tables.
    $sql = "SELECT TABLE_NAME FROM INFORMATION_SCHEMA.TABLES WHERE TABLE_NAME LIKE 'civicrm_%'";
    $dao = CRM_Core_DAO::executeQuery($sql);
    while($dao->fetch()) {
      $tables[] = $dao->TABLE_NAME;
    }
  }
  // Create a file name.
  $dir = '/var/www/powerbase/sites/default/files/csv-export-' . md5(rand());

  if(!mkdir($dir)) {
    drush_log("Failed to create directory.", 'error');
    return;
  }
  drush_log("Making directory: $dir", "ok");
  foreach($tables as $table) { 
    $sql = "SELECT COLUMN_NAME FROM INFORMATION_SCHEMA.COLUMNS WHERE TABLE_NAME = %0";
    $column_dao = CRM_Core_DAO::executeQuery($sql, array(0 => array($table, 'String')));
    $columns = array();

    while($column_dao->fetch()) {
      $columns[] = $column_dao->COLUMN_NAME;
    }

    // Create file for this table.
    $fp = fopen("${dir}/${table}.csv", 'w');

    // Add the column headers.
    fputcsv($fp, $columns);
    $sql = "SELECT * FROM " . $table;
    $row_dao = CRM_Core_DAO::executeQuery($sql);

    while($row_dao->fetch()) {
      reset($columns);
      $row = array();
      while(list(, $column) = each($columns)) {
        $row[] = $row_dao->$column;
      }
      fputcsv($fp, $row);
    }
  }
}

function drush_ptp_reset_drupal_email_address() {
  _civicrm_init();
  $site_mail = variable_get('site_mail');
  if(preg_match('/@progressivetech.org/', trim($site_mail))) {
    echo "match: " . $site_mail . "\n";
    // First try the default "From email addresses".
    $skipFatal = TRUE;
    $backupEmailValues = CRM_Core_BAO_Domain::getNameAndEmail($skipFatal);

    // getNameAndEmail returns an array with two empty values if no match
    // is found - and that is not considered "empty" by php.
    if ($backupEmailValues[1] == '') {
      $backupEmailValues = NULL;
    }
    if (empty($backupEmailValues)) {
      // If those are not set, try the system wide domain email address.
      $results = civicrm_api3('Domain', 'getsingle', array());
      $backupEmailValues = array(
        0 => $results['name'],
        1 => $results['domain_email'],
      );
    }
    if(!empty($backupEmailValues[1])) {
      echo "And the other setting is: " . $backupEmailValues[1] . "\n";
      variable_set('site_mail', $backupEmailValues[1]);

    }
  }
}

// Temp solution for: https://issues.civicrm.org/jira/browse/CRM-19851
function drush_ptp_recreate_multilingual_view() {
  _civicrm_init();
  $table = drush_get_option('table', NULL);
  $locale = drush_get_option('locale', NULL);

  if(empty($locale)) {
    $locales = array('en_US', 'es_MX');
  }
  else {
    $locales = array($locale);
  }
  if(empty($table)) {
    drush_log(dt("Please pass table."), 'error');
    return FALSE;
  }

  $class = 'CRM_Core_I18n_SchemaStructure';
  $dao = new CRM_Core_DAO();
  $columns =& $class::columns();
  $cols = array();
  $dao->query("DESCRIBE {$table}", FALSE);
  while ($dao->fetch()) {
    // view non-internationalized columns directly
    if (!in_array($dao->Field, array_keys($columns[$table])) and
      !preg_match('/_[a-z][a-z]_[A-Z][A-Z]$/', $dao->Field)
    ) {
      $cols[] = $dao->Field;
    }
  }
  foreach($locales as $locale) {
    // view intrernationalized columns through an alias
    foreach ($columns[$table] as $column => $_) {
      $cols[] = "{$column}_{$locale} {$column}";                           
    }                                                                      
    $sql = "CREATE OR REPLACE VIEW {$table}_{$locale} AS SELECT " . implode(', ', $cols) . "
  FROM {$table}";
    // Run through drupal - otherwise, CiviCRM will try to internationalize the table names.
    db_query($sql);
  }
}

function drush_ptp_set_outbound_mail_settings() {
  _civicrm_init();
  $host = drush_get_option('host','bulk.ourpowerbase.net'); 
  drush_log(dt("Setting outbound host to @host", array('@host' => $host)), 'ok');
  ptp_set_outbound_mail_settings($host);
}

/**
 * Import the given CSV file as options.
 *
 **/
function drush_ptp_import_options() {
  _civicrm_init();

  $location = drush_get_option('location', NULL);
  $action = drush_get_option('action', NULL);
  $name = drush_get_option('name', NULL);
  $title = drush_get_option('title', NULL);

  // Sanity checking.
  if(empty($location) || preg_match('/^https:/', $location)) {
    drush_log(dt("Please pass a location that starts with https:", 'error'));
    return FALSE;
  }
  $expected_actions = array('replace', 'add', 'new');
  if(!in_array($action, $expected_actions)) {
    drush_log(dt("Please pass replace, add or new as the action.", 'error'));
    return FALSE;
  }

  if($action == 'replace' || $action == 'add') {
    if(empty($name)) {
      drush_log(dt("The name is required if replace'ing or add'ing.", 'error'));
      return FALSE;
    }
  }
  else {
    if(empty($title)) {
      drush_log(dt("The title is required if creating a new option group.", 'error'));
      return FALSE;
    }
  }

  // If this is a new option group and no name is passed, auto-generate
  // one based on the title.
  if($action == 'new' && empty($name)) {
    $name = ptp_label_to_name($title);
  }

  // Now get and validate the CSV file.
  $lines = file($location);
  if(FALSE === $lines) {
    drush_log(dt("Failed to get the file."), 'error');
    return FALSE;
  }
  $options = array();
  foreach($lines as $line) {
    $out = str_getcsv(trim($line));
    if(empty($out)) {
      drush_log(dt("One of the rows in your CSV file didn't parse properly. Are you sure it is comma delimited?"), 'error');
      return FALSE;
    }
    $option_label = $out[0];
    $option_value = $out[1];
    $option_name = ptp_label_to_name($option_label);
    if(array_key_exists($option_name, $options)) {
      drush_log(dt("I found two labels that create the same name (when you remove all non-alpha numeric charactes). Please revise labels. Sorry I am too lazy to do this for you."), 'error');
      return FALSE;
    }
    $options[] = array('label' => $option_label, 'name' => $option_name, 'value' => $option_value);
  }
  // Ready, let's do it. Start by finding an option group.
  $option_group_id = NULL;
  try {
    // Create
    $params = array(
      'name' => $name,
    );
    $results = civicrm_api3('OptionGroup', 'get', $params);

    $count = $results['count'];
    // If we are add'ing or replace'ing, we should have a result. If we
    // are new'ing then we should not have a result.
    if($action == 'new') {
      if($count > 0) {
        drush_log(dt("An option group with the same name already exists (@name).", array('@name' => $name)), 'error');
        return FALSE;
      }
      // Otherwise, we should create it.
      $params = array(
        'name' => $name,
        'title' => $title,
        'is_active' => 1
      );
      $results = civicrm_api3('OptionGroup', 'create', $params);
      $option_group_id = $results['id'];
    }
    elseif($action == 'replace' || $action == 'add') {
      if($count == 0) {
        drush_log(dt("I can't find that option group (@name).", array('@name' => $name)), 'error');
        return FALSE;
      }
      $option_group_id = $results['id'];
    }

    drush_log(dt("Creating options for option_group_id @option_group_id.", array('@option_group_id' => $option_group_id)));
    while(list(, $option) = each($options)) {
      $params = array(
        'option_group_id' => $option_group_id,
        'label' => $option['label'],
        'name' => $option['name'],
        'value' => $option['value'],
        'is_active' => 1
      );
      civicrm_api3('OptionValue', 'create', $params);
    }
  }
  catch (CiviCRM_API3_Exception $e) {
    drush_log(dt("We hit a snag."), 'error');
    drush_log($e->getMessage());
    return FALSE;
  }
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
  $logins_field = 'custom_586';
  // custom_471 is count of contacts
  $contacts_field = 'custom_584';
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
    drush_log(dt("Unable to update the record.") . print_r($results), 'error');
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

function drush_ptp_find_stripe_subs_to_update() {
  _civicrm_init();
  // Get a list of the last 100 strip events
  $result = civicrm_api3('Stripe', 'listevents', [ 'ppid' => 7, 'limit' => 100 ]);
  foreach ($result['values'] as $value) {
    if ($value['processed'] == "no") {
      $evtid = $value['id'];
      $stripe_customer_id = $value['customer'];
      $stripe_subscription_id = $value['subscription'];
      // Lookup the customer so we can get the contact id.
      $customer = \Stripe\Customer::retrieve($stripe_customer_id);
      if (isset($customer->metadata->customer_id)) {
        $contact_id = $customer->metadata->customer_id;
        echo "Processing: $evtid : $stripe_subscription_id : $stripe_customer_id : $contact_id \n";
        // First, update their subscription from authorize to stripe
        drush_set_option('id', $contact_id);
        drush_set_option('sub', $stripe_subscription_id);
        drush_set_option('authid', 3);
        try {
          drush_ptp_authorize_to_stripe();
          // And now reproccess this event
          $result = civicrm_api3('Stripe', 'Ipn', [ 'evtid' => $evtid, 'ppid' => 7 ]);
          print_r($result);
        }
        catch (CiviCRM_API3_Exception $e) {
          drush_log("Problem! " . $e->getMessage(), 'error');
        }
      }
    }
   } 
 }
/**
 * move recurring contributions from authorize to stripe
 *
 */
function drush_ptp_authorize_to_stripe() {
  _civicrm_init();
  $id = drush_get_option('id', NULL);
  $sub = drush_get_option('sub', NULL);
  $authid = drush_get_option('authid', NULL);
  $stripeid = drush_get_option('stripeid', NULL);

  if (empty($id) || empty($sub)) {
    drush_log("Please provide both id and sub", 'error');
    return;
  }

  // Get the payment processor type ids
  if (empty($stripeid)) {
    $stripe_type_id = civicrm_api3('PaymentProcessorType', 'getvalue', [ 'name' => 'Stripe', 'return' => 'id' ]);
    $stripeid = civicrm_api3('PaymentProcessor', 'getvalue', [ 'payment_processor_type_id' => $stripe_type_id, 'is_test' => 0, 'return' => 'id' ]);
  }

  if (empty($authid)) {
    $authorize_type_id = civicrm_api3('PaymentProcessorType', 'getvalue', [ 'name' => 'AuthNet', 'return' => 'id']);
    $authid = civicrm_api3('PaymentProcessor', 'getvalue', [ 'payment_processor_type_id' => $authorize_type_id, 'is_test' => 0, 'return' => 'id' ]);
  }

  // Get the recurring contribution id associated with this contact.
  $params = [ 'contact_id' => $id, 'payment_processor_id' => $authid, 'contribution_status_id' => "In Progress" ];
  $recur = civicrm_api3('ContributionRecur', 'getsingle', $params );

  drush_log("Updating contribution recur id: " . $recur['id']);
  // Update it to use the stripe info.
  $recur['trxn_id'] = $sub;
  $recur['processor_id'] = $sub;
  $recur['payment_processor_id'] = $stripeid;

  civicrm_api3('ContributionRecur', 'create', $recur);

  // Check for membership record.
  // We may need to fix for this scenario: https://lab.civicrm.org/dev/membership/issues/13#note_25614
  $membership = civicrm_api3('Membership', 'getsingle', [ 'contribution_recur_id' => $recur['id'] ]);
  if ($membership['id']) {
    // Find the first contribution 
    $orig_contribution = civicrm_api3('Contribution', 'get', [ 'contribution_recur_id' => $recur['id'], 'options' => [ 'limit' => 1]] );
    $line_item = civicrm_api3('LineItem', 'getsingle', [ 'contribution_id' => $orig_contribution['id'] ] );
    // Update the line item so civi knows it is a membership
    if ($line_item['entity_table'] != 'civicrm_membership') {
      drush_log("Updating line item table");
      // You really can't do this via the API.
      $sql = "UPDATE civicrm_line_item SET entity_table = 'civicrm_membership' AND entity_id = %0 WHERE id = %1";
      $params = array(0 => array($membership['id'], 'Integer'), 1 => array($line_item['id'], 'Integer'));
      CRM_Core_DAO::executeQuery($sql, $params);
    }
  }
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
        "Error: user with @email email address already exists (uid: @uid).",
        array('@email' => $target_super_user_email, '@uid' => $uid)
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
      LIKE 'employee%' AND is_active = 1 LIMIT 1";
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
function ptp_set_outbound_mail_settings($host = 'bulk.ourpowerbase.net') {
  // Set the outbound email settings
  drush_log(dt("Setting outbound mail settings."));
  $sub = ptp_get_subdomain();
  if(preg_match('/(^training|^demo|clone)$/', $sub)) {
    // For training, demo or clone sites, redirect to the database.
    // We don't want to send email on these sites ever.
    $new['outBound_option'] = 5;  // 5 is redirect to db
  } elseif ( getenv('LONG_LIVE') == 'y' ) {
    $new['outBound_option'] = 0;  // 0 is for smtp
    $new['smtpServer'] = $host;
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
      'https://docs.civicrm.org/user/en/stable/',
    'CiviCRM Community Forums' =>
      'https://civicrm.stackexchange.com/',
    'PowerBase FAQs' =>
      'https://network.progressivetech.org/tips',
    'Video Tutorials' =>
      'https://www.youtube.com/user/PowerOnPTP/playlists',
    'Change my password' =>
      'user',
    'About' =>
      'https://ourpowerbase.net/',
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
      // Ignore if it has never been setup
      $status = sumfields_get_setting('generate_schema_and_data', FALSE);
      if(empty($status)) return;
      $site = ptp_get_subdomain();
      drush_log(dt("@site: Missing Contribution Trigger", array('@site' => $site)), 'error');
      if (drush_get_option('fix', FALSE)) {
         ptp_fix_sumfields();
      }
    }
  }
}

function ptp_fix_sumfields() {
  _civicrm_init();
  if (function_exists('sumfields_save_setting')) {
    sumfields_save_setting('generate_schema_and_data', 'scheduled:'. date('Y-m-d H:i:s'));
    $returnValues = array();
    $site = ptp_get_subdomain();
    if(!sumfields_gen_data($returnValues)) {
      drush_log(dt("@site: There was an error fixing the data.", array('@site' => $site)), 'error');
    }
    else {
      drush_log(dt("@site: Data fixed", array('@site' => $site)), 'error');
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

/**
 * Convert a given string to a name with onl alpha numberic and 
 * undescore characters.
 */
function ptp_label_to_name($label) {
  $name = strtolower($label);
  // Replace anything that is not a number or letter with an underscore.
  return preg_replace('[^a-z0-9]', '_', $name);
}


/**
 * Off the cuff helper for workingwa
 *
 */
function ptp_correlate_email_opens_to_contributions() {
  $group_id = 477;
  $group = new CRM_Contact_DAO_Group();
  $group->id = $group_id;
  $group->find(TRUE);
  $force = TRUE;
  // Check to see if lock exists
  $lock = Civi\Core\Container::singleton()->get('lockManager')->acquire("data.core.group.{$id}");
  if (!$lock->isAcquired()) {
    drush_log(dt("Lock exists, not rebuilding."), 'error');
    return;
  }
  CRM_Contact_BAO_GroupContactCache::load($group, $force);

  $sql = 'SELECT c.id, c.sort_name, MIN(co.receive_date) AS date FROM civicrm_contact c JOIN civicrm_contribution co ON c.id = co.contact_id JOIN civicrm_group_contact_cache gc ON c.id = gc.contact_id WHERE group_id = %0 AND co.contribution_status_id = 1 GROUP BY c.id';
  $dao = CRM_Core_DAO::executeQuery($sql, array(0 => array($group_id, 'Integer')));
  $fp = tmpfile();

  $fields = array('contact_id', 'name', 'first_contribution_date', 'opened_30_days_before', 'opened_30_days_after', 'click_30_days_before', 'click_30_days_after');
  // Put in the header row.
  fputcsv($fp, $fields);

  while($dao->fetch()) {
    $date_ts = strtotime($dao->date);
    $before_date = date('Y-m-d', $date_ts - (30 * 86400));
    $after_date = date('Y-m-d', $date_ts + (30 * 86400));

    // Check for opened.
    $sql = "SELECT time_stamp FROM civicrm_mailing_event_opened o JOIN civicrm_mailing_event_queue q ON o.event_queue_id = q.id AND contact_id = %0 AND time_stamp BETWEEN %1 AND %2 LIMIT 1";
    $params = array(
      0 => array($dao->id, 'Integer'),
      1 => array($before_date, 'String'),
      2 => array($dao->date, 'String')
    );
    $ddao = CRM_Core_DAO::executeQuery($sql, $params);
    $ddao->fetch();
    $opened_before = $dao->time_stamp;
    $params[1] = array($dao->date, 'String');
    $params[2] = array($after_date, 'String');
    $ddao = CRM_Core_DAO::executeQuery($sql, $params);
    $ddao->fetch();
    $opened_after = $ddao->time_stamp;

    // Check for clicked.
    $sql = "SELECT time_stamp FROM civicrm_mailing_event_trackable_url_open o JOIN civicrm_mailing_event_queue q ON o.event_queue_id = q.id AND contact_id = %0 AND time_stamp BETWEEN %1 AND %2 LIMIT 1";
    $params = array(
      0 => array($dao->id, 'Integer'),
      1 => array($before_date, 'String'),
      2 => array($dao->date, 'String')
    );
    $ddao = CRM_Core_DAO::executeQuery($sql, $params);
    $ddao->fetch();
    $click_before = $dao->time_stamp;
    $params[1] = array($dao->date, 'String');
    $params[2] = array($after_date, 'String');
    $ddao = CRM_Core_DAO::executeQuery($sql, $params);
    $ddao->fetch();
    $click_after = $ddao->time_stamp;
    
    fputcsv($fp, array($dao->id, $dao->sort_name, $dao->date, $open_before, $open_after, $click_before, $click_after));
  }
  rewind($fp);
  while (!feof($fp)) {
    echo fread($fp, 8192);
  }
  fclose($fp);
}

function ptp_find_possible_dupe_activities() {
  _civicrm_init();
  $sql = "SELECT a.id, subject, activity_type_id, date_add(created_date, INTERVAL 15 SECOND) as date_high, date_add(created_date, INTERVAL -15 SECOND) AS date_low FROM civicrm_activity a JOIN civicrm_option_value ov ON a.activity_type_id = ov.value JOIN civicrm_option_group og ON ov.option_group_id = og.id AND og.name = 'activity_type' WHERE created_date > '2019-06-01' AND ov.name NOT IN ('Membership Renewal', 'Membership Signup', 'Event Registration', 'Change Membership Status', 'Petition', 'Sign On')";
  $row = CRM_Core_DAO::executeQuery($sql);
  while($row->fetch()) {
    $sql = "SELECT id, subject, activity_type_id, created_date FROM civicrm_activity WHERE id != %0 AND activity_type_id = %1 AND subject = %2 AND created_date BETWEEN %3 and %4";
    $params = array(
      0 => array($row->id, 'Integer'),
      1 => array($row->activity_type_id, 'Integer'),
      2 => array($row->subject, 'String'),
      3 => array(str_replace(array(' ','-',':'),'', $row->date_low), 'Timestamp'),
      4 => array(str_replace(array(' ', '-', ':'), '', $row->date_high), 'Timestamp'),
    );
    $rrow = CRM_Core_DAO::executeQuery($sql, $params);
    while($rrow->fetch()) {
      echo "activity id: $rrow->id\n";
      echo "activity subject: $rrow->subject\n";
      echo "activity type id: $rrow->activity_type_id\n";
      echo "activity created: $rrow->created_date\n\n";

    }
  }
}
