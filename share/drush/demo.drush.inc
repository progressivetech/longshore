<?php

/**
 * @file
 *
 * Functions for creating demo data. 
 */

define('DEMO_PROTEST_EVENT_TITLE', 'Stop the ICE Raids: Street Protest');
define('DEMO_WORKSHOP_EVENT_TITLE', 'RESIST! Organizing Workshop');

/**
 * Implementation of hook_drush_command().
 */
function demo_drush_command() {
  $items['demo-data'] = 
    array(
      'description' => dt("Create demo data."),
      'options' => array(
        'seed' => "A short string to prepend to the passwords that will be generated."
      ),  
    );
  return $items;
}


function drush_demo_data() {
  

  _civicrm_init();
  try {
    demo_contacts();
    demo_campaign();
    demo_contribution_page();
    demo_price_set();
    demo_contributions();
    demo_membership_type();
    demo_memberships();
    demo_site_name();
    demo_email_templates();
    demo_interest_participation_status();
    demo_insert_logins();
    demo_create_reports();	
    demo_create_organizing_workshop_event();
    demo_create_protest_event();
    demo_create_participants();
    demo_add_images();
    demo_unset_default_footer();
    demo_create_random_activities();
    demo_save_sumfields();
  }
  catch (CiviCRM_API3_Exception $e) {
    drush_log(dt("Failed to create demo data in the database."), 'error');
    drush_log($e->getMessage(), 'error');
    return;
  }
}

function demo_save_sumfields() {
  $out = sumfields_get_setting("generate_schema_and_data", FALSE);
  if ($out) {
    drush_log(dt("Not setting sumfields, already set"), 'ok');
    return;
  }

  // Let's use all fields
  $new_active_fields = array(
    'contribution_total_lifetime',
    'contribution_total_this_year',
    'contribution_total_twelve_months',
    'contribution_total_deductible_this_year',
    'contribution_total_last_year',
    'contribution_total_deductible_last_year',
    'contribution_total_year_before_last',
    'contribution_total_deductible_year_before_last_year',
    'contribution_amount_last',
    'contribution_date_last',
    'contribution_date_first',
    'contribution_largest',
    'contribution_total_number',
    'contribution_average_annual_amount',
    'soft_total_lifetime',
    'soft_total_this_year',
    'soft_total_twelve_months',
    'contribution_date_last_membership_payment',
    'contribution_amount_last_membership_payment',
    'event_last_attended_name',
    'event_last_attended_date',
    'event_total',
    'event_attended',
    'event_attended_percent_total',
    'event_noshow',
    'event_noshow_percent_total',
    'event_turnout_attempts',
    'event_turnout_attended',
    'event_turnout_noshow',
    'event_attended_percent_turnout',
    'event_noshow_percent_turnout'
	);
	sumfields_save_setting('new_active_fields', $new_active_fields);

	$financial_type_ids = array(3,1);
  sumfields_save_setting('financial_type_ids', $financial_type_ids);

  $membership_financial_type_ids = array(2);
  sumfields_save_setting('membership_financial_type_ids', $membership_financial_type_ids);

	$event_type_ids = array(1,2,3,4,5,6);
  sumfields_save_setting('event_type_ids', $event_type_ids);

	$participant_status_ids = array(2);
  sumfields_save_setting('participant_status_ids', $participant_status_ids);

	$participant_noshow_ids = array(3);
  sumfields_save_setting('participant_noshow_ids', $participant_noshow_ids);

  $returnValues = array();

  sumfields_save_setting('generate_schema_and_data', 'scheduled:'. date('Y-m-d H:i:s'));
  drush_log("Generating summary fields...", 'ok');
  if (!sumfields_gen_data($returnValues)) {
    drush_log(dt("There was an error setting sumfields."), 'error');
  }

}
function demo_create_random_activities() {
  $results = civicrm_api3('Activity', 'get', array('activity_type_id' => 1));
  if($results['count'] > 0) {
    drush_log("Not creating random activities, already there.");
    return;
  }
  // Create 100 activities of the types: meeting (1), phone call (2)
  // linked to the lead abatement campaign
  for($i = 0; $i < 100; $i++) {
    $contact_id = rand(50,5000);
    $activity_type_id = rand(1,2);
    // Scheduled (1), completed (2), or cancelled (3)
    $status_id = array_rand(array(1 => 1, 2 => 2, 3 => 3));
    $subject = array_rand(array("Check in", "Reaching out", "Update", "Info meeting"));
    $engagement_level = rand(1,5);
    $time_high = time();
    $time_low = $time_high - (86400 * 365); // A year ago.
    $activity_date_time = date('Y-m-d H:i:s', rand($time_low, $time_high));
    $params = array(
      'source_contact_id' => 2,
      'target_contact_id' => $contact_id,
      'subject' => $subject,
      'engagement_level' => $engagement_level,
      'activity_date_time' => $activity_date_time,
      'status_id' => $status_id,
      'campaign_id' => 1,
      'activity_type_id' => $activity_type_id
    );
    civicrm_api3('Activity', 'create', $params);
  }
}

function demo_unset_default_footer() {
  // Since we are using a email template with the unsub links, we dont' want a footer
  // to be automatically set.
  $sql = "UPDATE civicrm_mailing_component SET is_default = 0 WHERE component_type = 'Footer'";
  CRM_Core_DAO::executeQuery($sql);
}

function demo_price_set() {
  $price_set = array(
		"name" => "donate_to_neighbors_for_justice",
		"title" => "Donate to Neighbors for Justice",
		"is_active" => "1",
		"extends" => "2",
		"financial_type_id" => "3",
		"is_quick_config" => "1",
		"is_reserved" => "0",
		"min_amount" => "0",
		"entity" =>  array(
			"civicrm_contribution_page" => array(1)
		),	
	);

	$result = civicrm_api3('PriceSet', 'get', array('name' => 'donate_to_neighbors_for_justice'));
	if($result['count'] == 1) {
		drush_log("Not creating price set, already exists.", 'ok');
		return;
	}
  else {
    $result = civicrm_api3('PriceSet', 'create', $price_set);
		$price_set_id = $result['id'];
    // How can this part be added via the API??
    $sql = "INSERT INTO civicrm_price_set_entity SET entity_table = 'civicrm_contribution_page', entity_id = 1, price_set_id = %0";
    CRM_Core_DAO::executeQuery($sql, array(0 => array($price_set_id, 'Integer')));
  } 

  $price_fields = array(
    array(
      "price_set_id" => $price_set_id,
      "name" => "contribution_amount",
      "label" => "Contribution Amount",
      "html_type" => "Radio",
      "is_enter_qty" => "0",
      "weight" => "2",
      "is_display_amounts" => "1",
      "options_per_line" => "1",
      "is_active" => "1",
      "is_required" => "0",
      "visibility_id" => "1"
	),
  array(	
    "price_set_id" => $price_set_id,
    "name" => "other_amount",
    "label" => "Other Amount",
    "html_type" => "Text",
    "is_enter_qty" => "0",
    "weight" => "3",
    "is_display_amounts" => "0",
    "options_per_line" => "1",
    "is_active" => "1",
    "is_required" => "0",
    "visibility_id" => "1"
    )
  );

  $price_field_ids = array();
  while(list(, $price_field) = each($price_fields)) {
    $result = civicrm_api3('PriceField', 'create', $price_field);
    $name = $price_field['name'];
    $price_field_ids[$name] = $result['id'];
  }

  $price_field_values = array(
    array(
      "price_field_id" => $price_field_ids['contribution_amount'],
      "name" => "Super",
      "label" => "Super",
      "amount" => "10",
      "weight" => "1",
      "is_default" => "0",
      "is_active" => "1",
      "financial_type_id" => "3",
      "non_deductible_amount" => "0.00",
      "contribution_type_id" => "3"
    ),
    array(
      "price_field_id" => $price_field_ids['contribution_amount'],
      "name" => "Awesome",
      "label" => "Awesome",
      "amount" => "100",
      "weight" => "2",
      "is_default" => "1",
      "is_active" => "1",
      "financial_type_id" => "3",
      "non_deductible_amount" => "0.00",
      "contribution_type_id" => "3"
    ),
    array(
      "price_field_id" => $price_field_ids['contribution_amount'],
      "name" => "Best",
      "label" => "Best",
      "amount" => "500",
      "weight" => "3",
      "is_default" => "0",
      "is_active" => "1",
      "financial_type_id" => "3",
      "non_deductible_amount" => "0.00",
      "contribution_type_id" => "3"
    ),
    array(
      "price_field_id" => $price_field_ids['other_amount'],
      "name" => "Other_Amount",
      "label" => "Other Amount",
      "amount" => "1",
      "weight" => "1",
      "is_default" => "0",
      "is_active" => "1",
      "financial_type_id" => "3",
      "non_deductible_amount" => "0.00",
      "contribution_type_id" => "3"
    ),
  );
  while(list(, $price_field_value) = each($price_field_values)) {
    civicrm_api3('PriceFieldValue', 'create', $price_field_value);
  }
}

function demo_add_images() {
  $dirs = array(
    '/var/www/powerbase/sites/default/files/civicrm/persist/contribute/',
    '/var/www/powerbase/sites/default/files/civicrm/persist/contribute/images/',
  );
  foreach($dirs as $dir) { 
    if(!file_exists($dir)) {
      mkdir($dir);
    }
  }
  $path = '/var/www/powerbase/sites/default/files/civicrm/persist/contribute/images/immigration-protest.jpg';
  if(file_exists($path)) {
    drush_log("Not saving image, already exists.", 'ok');
    return;
  }
  $url = 'https://upload.wikimedia.org/wikipedia/commons/thumb/6/62/2017.03.07_-MuslimBan_2.0_Protest%2C_Washington%2C_DC_USA_00789_%2833192329791%29.jpg/800px-2017.03.07_-MuslimBan_2.0_Protest%2C_Washington%2C_DC_USA_00789_%2833192329791%29.jpg';
  file_put_contents($path, file_get_contents($url));

}

function demo_create_participants() {
  $result = civicrm_api3('Participant', 'get', array());
  if($result['count'] > 0) {
    drush_log(dt("Not creating event participants, already exist."), 'ok');
    return;
  }

  // Get info about the events we expect to be in the database.
  $params = array(
    'return' => array('registration_start_date', 'id'),
    'title' => DEMO_WORKSHOP_EVENT_TITLE
  );
  $workshop = civicrm_api3( 'Event', 'getsingle', $params );
  $workshop_start_reg = $workshop['registration_start_date']; 
  $workshop_id = $workshop['id'];

   $params = array(
    'return' => array('registration_start_date', 'id'),
    'title' => DEMO_PROTEST_EVENT_TITLE
  );
  $protest = civicrm_api3( 'Event', 'getsingle', $params );
  $protest_start_reg = $workshop['registration_start_date']; 
  $protest_id = $workshop['id'];

  
  // We have membership from contact ids 800 - 850 and we have contributions
  // from contact ids 100 - 600.  We will pick a mix from both of these groups
  // to register to events so we can have complex searching options to find
  // potential leaders.
  
  // Ten members and ten contributors will be assigned to both events.
  for($i = 0; $i < 10; $i++) {
    foreach(array(100, 800) as $start) {
      $contact_id = $start + $i;
      // Event id 1 (workshop) is in the future, so we only have registered
      $status_id = 1; 
      $params = array(
        'contact_id' => $contact_id, 
        'event_id' => $workshop_id,
        'participant_status_id' => $status_id,
        'participant_register_date' => $workshop_start_reg,
        'participant_role_id' => 1,
      );
      civicrm_api3('Participant', 'create', $params);

      // Event id 2 is the protest, in the past.
      $params['event_id'] = $protest_id;
      // Status could be 1 (2 attended, 3 no-show, 4 cancelled);
      $params['participant_status_id'] = rand(2,4); 
      $params['participant_register_date'] = $protest_start_reg; 
      civicrm_api3('Participant', 'create', $params);
    }
  }

  // Now randomly add 100 people to one or the other events.
  $used_contact_ids = array(1 => array(), 2 => array());
  for($i = 0; $i < 100; $i++) {
    $contact_id = rand(50,5000);
    $event_id = array_rand(array(DEMO_PROTEST_EVENT_ID => null, DEMO_WORKSHOP_EVENT_ID => null));
    if(in_array($contact_id, $used_contact_ids[$event_id])) {
      // Avoid potential error of registering someone for some event
      // twice.
      continue;
    }
    if($event_id == DEMO_WORKSHOP_EVENT_ID) {
      $start_reg = $workshop_start_reg;
      $status_id = 1;
    }
    else {
      $status_id = rand(2,4);  
      $start_reg = $protest_start_reg;
    }
    $params = array(
      'contact_id' => $contact_id,
      'event_id' => $event_id,
      'participant_role_id' => 1,
      'participant_status_id' => $status_id,
      'participant_register_date' => $start_reg,
    );
    civicrm_api3('Participant', 'create', $params);
  }
}

function demo_create_protest_event() {
  $result = civicrm_api3('Event', 'get', array('title' => DEMO_PROTEST_EVENT_TITLE));
  if($result['count'] > 0) {
    drush_log(dt("Not creating demo event, already exists."), 'ok');
    return;
  }

  // This event will be in the past.
	// Start the event 30 days before the demo data is created.
	$start = date('Y-m-d H:i:s', time() - 86400 * 30);

  // End it 60 minutes later...
	$end = date('Y-m-d H:i:s', time() - (86400 * 30) + 3600);

  // Start reg 3 days earlier.
	$start_reg = date('Y-m-d H:i:s', time() - (86400 * 30) + 3600);

  // End reg when event starts
  $end_reg = $start;

	$params = array(
		"title" => "Stop the ICE Raids: Street Protest",
		"event_title" => "Stop the ICE Raids: Street Protest",
		"description" => "Stop ICE from destroying families",
		"event_description" => "Stop ICE from destroying families",
		"event_type_id" => "7", // Direct Action.
		"is_public" => "1",
		"start_date" => $start,
		"event_start_date" => $start,
		"end_date" => $end,
		"event_end_date" => $end,
		"is_online_registration" => "1",
		"registration_link_text" => "Register Now",
		"registration_start_date" => $start_reg,
		"registration_end_date" => $end_reg,
		"event_full_text" => "This event is currently full.",
		"is_monetary" => "0",
		"is_map" => "0",
		"is_active" => "1",
		"fee_label" => "Workshop Fee(s)",
		"is_show_location" => "1",
		"default_role_id" => "1",
		"intro_text" => "Register Now",
		"confirm_title" => "Confirm Your Registration Information",
		"is_email_confirm" => "0",
		"thankyou_title" => "Thank You for Registering",
		"is_pay_later" => "0",
		"pay_later_text" => "I will send payment by check",
		"pay_later_receipt" => "Please mail check or money order to => \r\n",
		"is_partial_payment" => "0",
		"is_multiple_registrations" => "0",
		"max_additional_participants" => "9",
		"allow_same_participant_emails" => "0",
		"has_waitlist" => "0",
		"requires_approval" => "0",
		"selfcancelxfer_time" => "0",
		"waitlist_text" => "This event is currently full. However you can register now and get added to a waiting list. You will be notified if spaces become available.",
		"is_template" => "0",
		"created_id" => "5005",
		"created_date" => date('Y-m-d H:i:s'),
		"currency" => "USD",
		"campaign_id" => "1",
		"is_share" => "1",
		"is_confirm_enabled" => "0",
		"is_billing_required" => "0"
	);

  $result = civicrm_api3('Event', 'create', $params);
}

function demo_create_organizing_workshop_event() {
  $result = civicrm_api3('Event', 'get', array('title' => DEMO_WORKSHOP_EVENT_TITLE));
  if($result['count'] > 0) {
    drush_log(dt("Not creating organizing event, already exists."), 'ok');
    return;
  }

	// Start the event 60 days from when the demo data is created.
	$start = date('Y-m-d H:i:s', time() + 86400 * 60);

  // End it 60 minutes later...
	$end = date('Y-m-d H:i:s', time() + (86400 * 60) + 3600);

  // Start reg now.
  $start_reg = date('Y-m-d H:i:s');

  // End reg when event starts
  $end_reg = $start;

	$params = array(
		"title" => "RESIST! Organizing Workshop",
		"event_title" => "RESIST! Organizing Workshop",
		"description" => "Joins for our RESIST! Organizing workshop",
		"event_description" => "Joins for our RESIST! Organizing workshop",
		"event_type_id" => "6",
		"is_public" => "0",
		"start_date" => $start,
		"event_start_date" => $start,
		"end_date" => $end,
		"event_end_date" => $end,
		"is_online_registration" => "1",
		"registration_link_text" => "Register Now",
		"registration_start_date" => $start_reg,
		"registration_end_date" => $end_reg,
		"event_full_text" => "This event is currently full.",
		"is_monetary" => "0",
		"is_map" => "0",
		"is_active" => "1",
		"fee_label" => "Workshop Fee(s)",
		"is_show_location" => "1",
		"default_role_id" => "1",
		"intro_text" => "Register Now",
		"confirm_title" => "Confirm Your Registration Information",
		"is_email_confirm" => "0",
		"thankyou_title" => "Thank You for Registering",
		"is_pay_later" => "0",
		"pay_later_text" => "I will send payment by check",
		"pay_later_receipt" => "Please mail check or money order to => \r\n",
		"is_partial_payment" => "0",
		"is_multiple_registrations" => "0",
		"max_additional_participants" => "9",
		"allow_same_participant_emails" => "0",
		"has_waitlist" => "0",
		"requires_approval" => "0",
		"selfcancelxfer_time" => "0",
		"waitlist_text" => "This event is currently full. However you can register now and get added to a waiting list. You will be notified if spaces become available.",
		"is_template" => "0",
		"created_id" => "5005",
		"created_date" => date('Y-m-d H:i:s'),
		"currency" => "USD",
		"campaign_id" => "1",
		"is_share" => "1",
		"is_confirm_enabled" => "0",
		"is_billing_required" => "0"
	);

  $result = civicrm_api3('Event', 'create', $params);
}


function demo_get_new_contributors_report_form_values() {
  return array(
    'entryURL' => CIVICRM_UF_BASEURL . '/civicrm/report/contribute/new?reset=1',
    'receive_date_op' => 'to',
    'receive_date_value' => date('m-d-Y'),
    'contribution_baseline_interval_op' => 'eq',
    'contribution_baseline_interval_value' => 3,
    'contribution_no_periods_op' => 'eq',
    'contribution_no_periods_value' => 4,
    'contribution_timeframe_min' => '',
    'contribution_timeframe_max' => '',
    'contribution_timeframe_op' => 'eq',
    'contribution_timeframe_value' => 120,
    'contribution_status_id_op' => 'in', 
    'contribution_status_id_value' => array('0' => 1),
    'description' => 'PowerBase Report displays how many contributors have given for the first time',
    'view_mode' => 'view',
    'addToDashboard' => 1,
    'cache_minutes' => 60,
    'permission' => 'access CiviReport',
    'charts' => 'barChart'
  );

}
function demo_get_key_numbers_report_form_values() {
	return array(
    'entryURL' => CIVICRM_UF_BASEURL . '/civicrm/report/contribute/keynumbers?reset=1',
    'report_options_op' => "in",
    'report_options_value' => array (
      0 => 'donor_number',
      1 => 'total_amount',
      2 => 'total_amount__individual',
      3 => 'average_donation__individual',
      4 => 'no_increased_donations__individual',
      5  => 'current_sustainer_count',
    ),
    'receive_date_relative' => 'current.year',
    'contribution_status_id_op' => 'in',
    'contribution_status_id_value' => array( '0' => 1),
    'financial_type_id_op' => 'in',
    'financial_type_id_value' => array(),
    'payment_instrument_id_op' => 'in',
    'payment_instrument_id_value' => array(),
    'description' => 'PowerBase Report displays Key performance Indicators',
    'addToDashboard' => 1,
    'cache_minutes' => 60,
    'permission' => 'access CiviReport'
  );
}

function demo_create_reports() {
  $key_numbers = array(
    'domain_id' => 1,
    'title' => "Key Numbers Report " . date('Y'),
    'report_id' => "contribute/keynumbers",
    'description' => "PowerBase Report displays Key performance Indicators",
    'permission' => 'access CiviReport',
    'form_values' => serialize(demo_get_key_numbers_report_form_values()),
    'header' => '',
    'footer' => '',
    'is_reserved' => 0,
  );

  $result = civicrm_api3('ReportInstance', 'get', array(
    'sequential' => 1,
    'title' => $key_numbers['title']
  ));

  if($result['count'] == "0") {
    civicrm_api3('ReportInstance', 'create', $key_numbers);
  }
  else {
    drush_log(dt("Not adding key numbers report, already there."), 'ok');
  }

  $new_contributors = array(
    'domain_id' => 1,
    'title' => "New Contributors by quarter",
    'report_id' => "contribute/new",
    'description' => "PowerBase Report displays how many contributors have given for the first time",
    'permission' => 'access CiviReport',
    'form_values' => serialize(demo_get_new_contributors_report_form_values()),
    'header' => '',
    'footer' => '',
    'is_reserved' => 0,
  );

  $result = civicrm_api3('ReportInstance', 'get', array(
    'sequential' => 1,
    'title' => $new_contributors['title']
  ));

  if($result['count'] == "0") {
    civicrm_api3('ReportInstance', 'create', $new_contributors);
  }
  else {
    drush_log(dt("Not adding new contributors report, already there."), 'key');
  }

}
function demo_insert_logins() {
  $seed = drush_get_option('seed', NULL);

  $sql = "SELECT uid FROM users WHERE uid > 1";
  $dao = CRM_Core_DAO::executeQuery($sql);

  $initial_count = $dao->N;
  // If there are existing records and $seed is passed, then delete those
  // records so they will be created again.
  if($initial_count > 0 && $seed) {
    drush_log("Deleting " . $dao->N . " user(s)");
    while($dao->fetch()) {
      user_delete($dao->uid);
    }
  }
  // If there are no records, and no seed is passed, it's an error.
  if($initial_count == 0 && !$seed) {
    drush_log(dt("Please pass --seed='something' to pass a string to seed the password string with. Replace something with something better."), 'error');
    return;
  }

  // If records exists and no seed is passed then skip creating users.
  if($initial_count > 0 && !$seed) {
    drush_log(dt("Not re-creating users."), 'ok');
    return;
  }

  $password = "${seed}-" . date('Y');
  drush_log("Adding users with names: fun1, fun2, adv1, adv2 etc and password: ${password}");
  $i = 1;
  $userversion = drush_user_get_class();
  $user = array(
    'pass' => $password,
    'access' => '0',
    'status' => 1,
  );

  $uids = array();
  while($i < 31) {
    $user['name'] = 'fun' . $i;
    $user['mail'] = $user['name'] . '@example.net';
    $account = $userversion->create($user);
    if($account->id()) {
      $uids['fun'][] = $account->id();
    }
    $user['name'] = 'adv' . $i;
    $user['mail'] = $user['name'] . '@example.net';
    $account = $userversion->create($user);
    if($account->id()) {
      $uids['adv'][] = $account->id();
    }
    $i++;
  }  
  while(list($type, $collection) = each($uids)) {
    while(list(, $uid) = each($collection)) {
      if($type == 'fun') {
        $role_name = 'PowerBase User';
      }
      elseif($type == 'adv') {
        $role_name = 'PowerBase Administrator';
      }
      else {
        drush_log("Unknown type: $type", 'error');
        return;
      }
      if ($role = user_role_load_by_name($role_name)) {
        user_multiple_role_edit(array($uid), 'add_role', $role->rid);
      }  
    }
  }  
  


}

function demo_interest_participation_status() {
  $result = civicrm_api3('ParticipantStatusType', 'get', array(
    'sequential' => 1,
    'name' => "Interested",
  ));
  if($result['count'] == "0") {
    drush_log(dt("Not adding interested participant status, already there."), 'ok');
    return;
  }
  $result = civicrm_api3('ParticipantStatusType', 'create', array(
    'name' => 'Interested',
    'label' => 'Interested',
    'is_reserved' => '0',
    'is_active' => '1',
    'is_counted' => '0',
    'class' => 'Positive'
  ));
}

function demo_email_templates() {
  // use msg_title as array key to help with dupe checking when adding
  $base_dir = __dir__ . '/templates';
  $templates = array(
    'Powerbase Simple Responsive' => array(
      'msg_title' => 'Powerbase Simple Responsive',
      'msg_subject' => '',
      'msg_text' => '',
      'msg_html' => file_get_contents($base_dir . '/powerbase-simple-responsive.html'),
      'is_active' => 1,
    ),
  );
  $disable_templates = array(
    'Powerbase Responsive Email One Column',
    'Powerbase Responsive Email One Column No Graphics',
    'Powerbase Responsive Email Two Column',
    'Powerbase Thank You Letter Template v1'
  );
  pbd_add_message_templates($templates, $disable_templates);
}

function demo_site_name() {
  // Drupal is easy.
  $name = 'PowerBase Training and Demo';
  $email = 'support@progressivetech.org';
  $contact_id = 1;
  $domain_id = 1;
  variable_set('site_name', $name);

  // CiviCRM is hard...
  //
  $defaultLocationType = CRM_Core_BAO_LocationType::getDefault();
  $params = array(
    'name' => $name,
    'entity_id' => $domain_id,
    'entity_table' => CRM_Core_BAO_Domain::getTableName(),
    'email_name' => $name,
    'email_address' => $email,
    'email' => array(
      1 => array(
        'email' => $email,
        'location_type_id' => $defaultLocationType->id,
      ),
    ),
  );
  $domain = CRM_Core_BAO_Domain::edit($params, $domain_id);

  $params += array('contact_id' => $contact_id);
  $contactParams = array(
    'sort_name' => $domain->name,
    'display_name' => $domain->name,
    'legal_name' => $domain->name,
    'organization_name' => $domain->name,
    'contact_id' => $contact_id,
    'contact_type' => 'Organization',
  );

  CRM_Contact_BAO_Contact::add($contactParams);
  CRM_Core_BAO_Location::create($params, TRUE);

  CRM_Core_BAO_Domain::edit($params, $domain_id);

  //set domain from email address, CRM-3552
  $emailName = '"' . $params['email_name'] . '" <' . $params['email_address'] . '>';

  $emailParams = array(
    'label' => $emailName,
    'description' => '',
    'is_active' => 1,
    'is_default' => 1,
  );

  $groupParams = array('name' => 'from_email_address');

  $optionValues = array();
  $default_from_address_option_value_id = NULL;
  CRM_Core_OptionValue::getValues($groupParams, $optionValues);
  foreach ($optionValues as $Id => $value) {
    if ($value['is_default'] && $value['is_active']) {
      $default_from_address_option_value_id = $Id;
    }
  }

  //add from email address.
  $action = CRM_Core_Action::ADD;
  $grpId = CRM_Core_DAO::getFieldValue('CRM_Core_DAO_OptionGroup', 'from_email_address', 'id', 'name');
  $fieldValues = array('option_group_id' => $grpId);
  $emailParams['weight'] = CRM_Utils_Weight::getDefaultWeight('CRM_Core_DAO_OptionValue', $fieldValues);

  //reset default within domain.
  $emailParams['reset_default_for'] = array('domain_id' => CRM_Core_Config::domainID());

  CRM_Core_OptionValue::addOptionValue($emailParams, $groupParams, $action, $default_from_address_option_value_id);
}

function demo_membership_type() {

  $results = civicrm_api3('MembershipType', 'get', array());
  if($results['count'] >= 1) {
    drush_log(dt("Not adding membership type, already there."), 'ok');
    return;
  }

  // Enter Member Organization first.
  $member_organization_name = 'Demo Organizing Project';
  $results = civicrm_api3('Contact', 'get', array('organization_name' => $member_organization_name));
  if($results['count'] == 1) {
    drush_log(dt("Not adding member org contact, already there."), 'ok');
    return;
  }

  $params = array(
    'contact_type' => 'Organization',
    'organization_name' => $member_organization_name
  );
  $result = civicrm_api3('Contact', 'create', $params);
  $member_contact_id = $result['id'];
 
  $params = array(
     "domain_id" => "1",
    "name" => "Demo Organizing Member",
    "description" => "Member of the Demo Organizing Project",
    "member_of_contact_id" => $member_contact_id,
    "financial_type_id" => "2",
    "minimum_fee" => "5.00",
    "duration_unit" => "year",
    "duration_interval" => "1",
    "period_type" => "rolling",
    "visibility" => "Public",
    "weight" => "1",
    "is_active" => "1"
  );
  civicrm_api3('MembershipType', 'create', $params);
}

function demo_memberships() {
  $results = civicrm_api3('Membership', 'get', array());
  if($results['count'] >= 1) {
    drush_log(dt("Not adding memberships, already there."), 'ok');
    return;
  }


  // We're going to add 50 members for contacts with ids 800 - 850
  // They are all going to $10 to be members. We have to create
  // a contribution record, and membership record and a line item
  // to link them together.

  $contact_id = 800;
  while($contact_id < 851) {
    // First the contribution. Let's keep it simple - everyone pays by check.
    $join_date = demo_random_date_in_last_year();
    $params = array(
        "contact_id" => $contact_id,
        "currency" => "USD",
        "receive_date" => $join_date,
        "total_amount" => '10',
        "fee_amount" => "",
        "net_amount" => "",
        "trxn_id" => "",
        "is_test" => "0",
        "is_pay_later" => "0",
        "contribution_status_id" => 1,
        "check_number" => rand(100,5000),
        "payment_instrument_id" => 4,
        "financial_type_id" => 2 // Member dues 
    );
    $result = civicrm_api3('Contribution', 'create', $params);
    $contribution_id = $result['id'];

    // Now the membership record.
    $params = array(
       "contact_id" => $contact_id,
      "membership_type_id" => "1",
      "join_date" => $join_date,
      "start_date" => $join_date,
    );  
    $result = civicrm_api3('membership', 'create', $params);
    $membership_id = $result['id'];

    // The membership activity date (which controls the membership dashboard
    // that shows when memberships came in) is improperly set to todays
    // date. Let's change it to the actual date
    $sql = "UPDATE civicrm_activity SET activity_date_time = %0 WHERE source_record_id = %1";
    $sql_params = array(0 => array($join_date, 'String'), array($membership_id, 'Integer'));
    CRM_Core_DAO::executeQuery($sql, $sql_params );

    // Lastly create the line item to connect the dots
    $params = array(
      "entity_table" => "civicrm_membership",
      "entity_id" => $membership_id,
      "label" => "Demo Organizing Member",
      "qty" => "1.00",
      "unit_price" => "10.00",
      "line_total" => "10.00",
      "financial_type_id" => "2",
      "contribution_id" => $contribution_id,
    );
    $result = civicrm_api3('LineItem', 'create', $params);
    $contact_id++;
  }
}

function demo_custom_id_for_name($col_name) {
  $result = civicrm_api3('CustomField', 'getsingle', array('column_name' => $col_name));
  return $result['id'];
}

function demo_get_random_issue_interest() {
  $options = array(
    'Economic Justice' => null,
    'Environmental Justice' => null,
    'Immigrant Rights' => null,
    'Workers Rights' => null
  );
  return array_rand($options);
}

function demo_get_random_constituent_type() {
  $options = array(
    'Volunteer' => null,
    'Activist' => null,
    'Member' => null,
    'Donor' => null,
    'Consultant' => null,
    'Ally' => null,
    'Elected Official' => null,
    'Board Member' => null,
    'Lead Organizer' => null,
    'Intern' => null,
    'Staff' => null,
    'Media_Contact' => null,
    'Funder' => null,
  );
  return array_rand($options);
}
function demo_contacts() {
  // contacts.csv downloaded from https://www.forsyth.cc/elections/voter_data.aspx
  // Only run if we have less than 2 contacts already.
  $sql = "SELECT COUNT(*) AS count FROM civicrm_contact";
  $dao = CRM_Core_DAO::executeQuery($sql);
  $dao->fetch();
  if ($dao->count > 3) {
    drush_log(dt("Not running, more than 3 contacts already entered."), 'ok');
    return;
  }
  if (($handle = fopen(__dir__  . "/contacts.csv", "r")) !== FALSE) {
    // Until skipping geo coding is fixed...
    // https://support.ourpowerbase.net/ticket/2292
    $params = array(
      'geoProvider' => "",
      'mapProvider' => "",
    );
    civicrm_api3('Setting', 'create', $params); 
    $i = 0;
    while (($data = fgetcsv($handle, 1000, ",")) !== FALSE) {
      $i++;
      if($i === 1) {
        // First row is the header, skip
        continue;
      }
      if ($i % 100 == 0) {
        drush_log(dt("Adding $i Contact", array('@i' => $i)), 'ok');
      }
      // name is in format: "LAST, FIRST MIDDLE"
      $name = explode(',', $data[1]);

      $params = array(
        'skip_geocode' => true,
        'contact_type' => 'Individual'
      );
      $params['last_name'] = trim(ucwords(strtolower(array_shift($name))));
      $remaining_names = explode(' ', $name[0]);
      $params['first_name'] = trim(ucwords(strtolower(array_pop($remaining_names))));

      /**
       * Voter ifo
       **/
      // precinct (precinct code)
      $id = 'custom_' . demo_custom_id_for_name('precinct');
      $params[$id] = $data[11];
      // city district (municipal district)
      $id = 'custom_' . demo_custom_id_for_name('city_district');
      $params[$id] = $data[12];
      // congressional district
      $id = 'custom_' . demo_custom_id_for_name('congressional_district');
      $params[$id] = $data[14];
      // state assembly (NC house)
      $id = 'custom_' . demo_custom_id_for_name('state_assembly_district');
      $params[$id] = $data[18];
      // state senate (NC senate)
      $id = 'custom_' . demo_custom_id_for_name('state_senate_district');
      $params[$id] = $data[17];
      // school district
      $id = 'custom_' . demo_custom_id_for_name('school_district');
      $params[$id] = $data[20];

      // Issue options
      $id = 'custom_' . demo_custom_id_for_name('issues_interest');
      $params[$id] = demo_get_random_issue_interest();

      // Constituent type
      $id = 'custom_' . demo_custom_id_for_name('constituent_type');
      $params[$id] = demo_get_random_constituent_type();

       // City State Zip is in format: CITY, STATE ZIP
      $street_address = $data[9];
      $city_state_zip = explode(',', $data[10]);
      $city = trim(array_shift($city_state_zip));
      $state_zip = explode(' ', trim($city_state_zip[0]));
      $state = trim(array_shift($state_zip));
      $postal_code = $state_zip[0];


      $params['api.address.create'] = array(
        'street_address' => $street_address,
        'city' => $city,
        'state_province_id' => 'North Carolina',
        'postal_code' => $postal_code,
        'is_primary' => 1,
        'location_type_id' => 1
      );
      $email_address = $params['first_name'] . '.' .  $params['last_name'] . 
        substr(md5(rand()), 0, 5) . '@example.net';
      $params['api.email.create'] = array(
        'email' => $email_address,
        'is_primary' => 1,
        'location_type_id' => 1
      );

      civicrm_api3('Contact', 'create', $params);
    }
    fclose($handle);
    $params = array(
      'geoProvider' => "Google",
      'mapProvider' => "Google",
    );
    civicrm_api3('Setting', 'create', $params); 
  }
}

function demo_campaign() {
  // Try to be indempotent.
  $results = civicrm_api3('Campaign', 'get', array());
  if($results['count'] >= 1) {
    drush_log(dt("Not adding campaign, already there."), 'ok');
    return;
  }    
  $params = array(
    "name" => "Lead_Poisoning",
    "title" => "Lead Poisoning Abatement",
    "start_date" => "2011-09-01",
    "campaign_type_id" => "3",
    "status_id" => "2",
    "is_active" => "1",
    "created_id" => "3",
    "created_date" => date('Y-m-d'),
    "last_modified_id" => "3",
    "last_modified_date" => date('Y-m-d')
  );

  $results = civicrm_api3('Campaign', 'create', $params);
}

function demo_random_date_in_last_year() {
  // Set parameters for random dates.
  $min_epoch = strtotime("1 year ago");
  $max_epoch = strtotime("today");

  $rand_epoch = rand($min_epoch, $max_epoch);
  return date('Y-m-d H:i:s', $rand_epoch);
}

function demo_random_donation_amount() {
  // Donations aren't that random...
  $possible = array(
    '10', '10', '10', '25', '25', '25', '25', '25', '25', '25', '25' ,'25', 
    '50', '50', '50', '50', '50', '50', '50', '50', '50', '50', '50', '50',
    '50', '50', '50', '50', '50', '75', '75', '75', '75', '100','100', '100',
    '100', '100', '100', '500', '500', '1000');
  return $possible[array_rand($possible)]; 
}

function demo_contributions() {
  $results = civicrm_api3('Contribution', 'get', array());
  if($results['count'] >= 1) {
    drush_log(dt("Not adding contributions, already there."), 'ok');
    return;
  }

  $campaign_id = civicrm_api3('Campaign', 'getvalue', array('return' => 'id'));
  $contribution_page_id = civicrm_api3('ContributionPage', 'getvalue', array('return' => 'id'));

  // We assume we have contacts in the database and that the contact ids
  // span consecutively from 100 - 600. 
  // We will create contributions for those 500 contacts.
  $start = 100;
  $end = 601;
  $i = $start;
 
  while($i < $end) {
    if ($i % 100 == 0) {
      drush_log(dt("Adding $i Contribution", array('@i' => $i)), 'ok');
    }
  
    // One out of three are checks, the rest credit cards.
    if(rand(1, 3) == 3) {
      // Check
      $trxn_id = NULL;
      $payment_instrument_id = 4;
      $check_number = rand(1,4000);
      $contribution_status_id = 1;
    }
    else {
      // Credit cardd.
      $trxn_id = substr(md5(rand()), 0, 12);
      $payment_instrument_id = 1;
      $check_number = NULL;

      // One out of ten are failed
      if(rand(1, 10) == 10) {
        $contribution_status_id = 4;
      }
      else {
        $contribution_status_id = 1;
      }
    }

    $params = array(
      "contact_id" => $i,
      "currency" => "USD",
      "receive_date" => demo_random_date_in_last_year(),
      "non_deductible_amount" => "0.00",
      "total_amount" => demo_random_donation_amount(),
      "fee_amount" => "",
      "net_amount" => "",
      "trxn_id" => $trxn_id,
      "is_test" => "0",
      "is_pay_later" => "0",
      "contribution_status_id" => $contribution_status_id,
      "check_number" => $check_number,
      "contribution_campaign_id" => $campaign_id,
      "contribution_page_id" => $contribution_page_id,
      "contribution_recur_id" => "",
      "payment_instrument_id" => $payment_instrument_id,
      "financial_type_id" => 1 // Donation.
    );
    civicrm_api3('Contribution', 'create', $params);
    $i++;
  }
}

function demo_contribution_page() {
  $results = civicrm_api3('ContributionPage', 'get', array());
  if($results['count'] >= 1) {
    drush_log(dt("Not adding contribution page, already there."), 'ok');
    return;
  }
  // First the contribution page
  $params = array(
    "title" => "Donate to Neighbors for Justice",
    "intro_text" => "\r\n\tHere's your chance to put your money where your mouth is. Your contribution makes it possible for us to continue working for environmental safety and justice in our community.\n",
    "financial_type_id" => "3",
    "is_credit_card_only" => "0",
    "is_monetary" => "1",
    "is_recur" => "0",
    "is_confirm_enabled" => "1",
    "is_recur_interval" => "0",
    "is_recur_installments" => "1",
    "adjust_recur_start_date" => "0",
    "is_pay_later" => "1",
    "pay_later_text" => "I will send payment by check",
    "pay_later_receipt" => "Send a check to our office at 123 4th Street, Anytown, XX",
    "is_partial_payment" => "0",
    "is_allow_other_amount" => "1",
    "min_amount" => "1.00",
    "thankyou_title" => "Thanks for Your Donation",
    "thankyou_text" => "\r\n\tYour donation is an investment in this community. We will use your contribution towards keeping our community safe.",
    "is_email_receipt" => "1",
    "receipt_from_name" => "Your Friendly Organizer",
    "receipt_from_email" => "info@example.org",
    "receipt_text" => "Thanks for your generous contribution.",
    "is_active" => "1",
    "amount_block_is_active" => "1",
    "start_date" => date('Y-m-d'),
    "created_date" => date('Y-m-d'),
    "currency" => "USD",
    "campaign_id" => "1",
    "is_share" => "1",
    "is_billing_required" => "0",
    "contribution_type_id" => "3"
  );
 
  $result = civicrm_api3('ContributionPage', 'create', $params);
}

