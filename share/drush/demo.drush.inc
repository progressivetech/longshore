<?php

/**
 * @file
 *
 * Functions for creating demo data. 
 */



/**
 * Implementation of hook_drush_command().
 */
function demo_drush_command() {
  $items['demo-data'] = 
    array(
      'description' => dt("Create demo data."),
    );
  return $items;
}


function drush_demo_data() {
	_civicrm_init();
  try {
    demo_contacts();
		demo_campaign();
    demo_contribution_page();
    demo_contributions();
		demo_membership_type();
		demo_memberships();
    demo_site_name();
    demo_email_templates();
  }
  catch (CiviCRM_API3_Exception $e) {
    drush_log(dt("Failed to create demo data in the database."), 'error');
    drush_log($e->getMessage(), 'error');
    return;
  }
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
		drush_log(dt("Not adding membership type, already there."), 'error');
		return;
	}

	// Enter Member Organization first.
	$member_organization_name = 'Demo Organizing Project';
	$results = civicrm_api3('Contact', 'get', array('organization_name' => $member_organization_name));
	if($results['count'] == 1) {
		drush_log(dt("Not adding member org contact, already there."), 'error');
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
		drush_log(dt("Not adding memberships, already there."), 'error');
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
    echo "$sql\n";
    print_r($sql_params);

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

function demo_contacts() {
  // contacts.csv downloaded from https://www.forsyth.cc/elections/voter_data.aspx
  // Only run if we less than 2 contacts already.
  $sql = "SELECT COUNT(*) AS count FROM civicrm_contact";
  $dao = CRM_Core_DAO::executeQuery($sql);
  $dao->fetch();
  if ($dao->count > 3) {
    drush_log(dt("Not running, more than 3 contacts already entered."), 'error');
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
    drush_log(dt("Not adding campaign, already there."), 'error');
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
    drush_log(dt("Not adding contributions, already there."), 'error');
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
    drush_log(dt("Not adding contribution page, already there."), 'error');
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


