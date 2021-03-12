<?php

/**
 * @file
 *
 * This scripts ensures that all payment plans in your CiviCRM
 * table are properly created in Stripe. For more information see
 * https://github.com/drastik/com.drastikbydesign.stripe/issues/103
 * and https://support.ourpowerbase.net/ticket/1620
 **/

function stripe_drush_command() {
  $items = array();
  $items['stripe-sync-plans'] =
    array('description' => "Ensure all stripe recurring payment plans in database are configured in stripe.");
  $items['stripe-fix-customers'] =
    array('description' => "Ensure all stripe customers are assigned to the right ppid.");
  $items['stripe-add-missing-customers'] = array(
    'description' => "Ensure a stripe customer record is created for all active recurring contributions",
    'options' => [
      'ppid' => "The stripe processor id"
    ]
  );
  $items['stripe-repair-subs'] = array(
    'description' => "Given a file with a list of subscription/customer pairs, try to re-create the right data so they get processed.",
    'options' => array(
      'file' => "The file with the colon separated subscription/customer pairs",
      'ppid' => "The stripe processor id if more than one."
    ),
  );
  $items['stripe-retry'] = array(
    'description' => "Re-try all transactions that are pending in Powerbase but succeeded in stripe.",
    'options' => array(
      'ppid' => "The payment processor id",
      'limit' => "The number of previous transactions to search"
    ),
  );
  return $items;
}

function drush_stripe_retry() {
  _civicrm_init();
  $ppid = drush_get_option('ppid', NULL);
  $limit = drush_get_option('limit', 100);
  $params = [
    'ppid' => $ppid,
    'limit' => $limit,
    'filter_processed' => 1,
    'source' => 'systemlog'
  ];
  $results = civicrm_api3('Stripe', 'ListEvents', $params);
  $total = count($results['values']);
  $unprocessed_success = 0;
  $unprocessed_fail = 0;
  foreach($results['values'] as $value) {
    if ($value['processed'] == 'no' && isset($value['system_log_id'])) {
      drush_log("Processing system log id " . $value['system_log_id']);
      $params = [ 'ppid' => $ppid, 'id' => $value['system_log_id'] ];
      $ipn_results = civicrm_api3('Stripe', 'Ipn', $params);
      if ($ipn_results['is_error'] == 0) {
	      $unprocessed_success++;
        drush_log("No error");
      }
      else {
	      $unprocessed_fail++;
        drush_log("Error!", 'error');
	      drush_log(print_r($ipn_results, TRUE), 'error');
      }	
    }
  }
  drush_log("Total: $total");
  drush_log("Total Unprocessed (success): $unprocessed_success");
  drush_log("Total Unprocessed (fail): $unprocessed_fail");
}

function drush_stripe_add_missing_customers() {
  $ppid = drush_get_option('ppid', NULL);

  if (empty($ppid)) {
    drush_log("Please pass ppid.", 'error');
    return;
  }
  // Get a list of the last month's recurring contributions so we will have the subscription and customer
  // ids.
  $result = civicrm_api3('Stripe', 'ListEvents', [ 'ppid' => $ppid, 'limit' => 100 ]);
  $stripe_data = $result['values'];
  // Iterate over list of active recurring contributions for which we have a valid subscription id.
  $sql = "SELECT DISTINCT contact_id, trxn_id FROM civicrm_contribution_recur WHERE trxn_id LIKE 'sub_%' AND contribution_status_id = 5";
  $dao = CRM_Core_DAO::executeQuery($sql);
  while($dao->fetch()) {
    $contact_id = $dao->contact_id;
    $trxn_id = $dao->trxn_id;
    $found = FALSE;
    $reprocess_events = [];
    foreach($stripe_data as $event) {
      if ($event['subscription'] == $trxn_id) {
	      $found = TRUE;
        // We have a match.
				if ($event['processed'] == 'no') {
					$reprocess_events[] = $event['id'];
				}
				// See if it exists already.
				$sql = "SELECT COUNT(*) AS count FROM civicrm_stripe_customers WHERE id = %0";
				$cdao = CRM_Core_DAO::executeQuery($sql, [ 0 => [ $event['customer'], 'String' ]]);
				$cdao->fetch();
				if ($cdao->count > 0) {
					continue;
				}

				$sql = "INSERT INTO civicrm_stripe_customers VALUES(%0, %1, NULL, %2)";
				$args = [ 0 => [ $event['customer'], 'String' ], 1 => [ $contact_id, 'Integer' ], 2 => [ $ppid, 'Integer' ] ] ;
				drush_log($sql . ";" .  $event['customer'] . ":" . $contact_id . ":" . $ppid, 'info');
				CRM_Core_DAO::executeQuery($sql, $args);
				
      }
    }
    if ($found === FALSE) {
      drush_log("Failed to insert customer for $contact_id", 'error');
    }
    else {
      foreach($reprocess_events as $evtid) {
        drush_log("Reprocseeing $evtid", 'info');
        civicrm_api3('Stripe', 'Ipn', [ 'evtid' => $evtid, 'ppid' => $ppid ]);
      }
    }
  }
}

function drush_stripe_fix_customers() {
  _civicrm_init();
  // First, get a list of all payment processor ids
  $sql = "SELECT pp.id, password FROM civicrm_payment_processor pp JOIN civicrm_payment_processor_type pt ON pp.payment_processor_type_id = pt.id WHERE pt.name = 'Stripe'";
  $ppids = [];
  $dao = CRM_Core_DAO::executeQuery($sql);
  while ($dao->fetch()) {
    $ppids[$dao->id] = $dao->password;
  }

  // Iterate over all customers and search for the customer id in the pp id it's coded to
  $sql = "SELECT id, processor_id, email FROM civicrm_stripe_customers ORDER BY processor_id";
  $dao = CRM_Core_DAO::executeQuery($sql);

  $seen = [];
  $not_found = [];
  while($dao->fetch()) {
    $customer_id = $dao->id;
    $processor_id = $dao->processor_id;

    if (!in_array($processor_id, $seen )){
      drush_log("Checking processor id $processor_id");
      $apikey = $ppids[$processor_id];
      \Stripe\Stripe::setApiKey($apikey);
      $seen[] = $processor_id;
    }

    //drush_log("Looking for customer $customer_id");
    echo ".";
    try {
      $cust = \Stripe\Customer::retrieve($customer_id);
      // drush_log("Found customer $customer_id");
      if(isset($cust->deleted) && $cust->deleted) {
        drush_log("customer $customer_id is deleted");
      }
    }
    catch (Stripe\Error\InvalidRequest $e) {
      drush_log("Could not find customer $customer_id in $processor_id");
      if (!array_key_exists($customer_id, $not_found)) {
        $not_found[$customer_id] = [
          'processor_ids' => [],
          'email' => $dao->email,
        ];
      }
      $not_found[$customer_id]['processor_ids'][] = $processor_id;
    }
  }

  // Now iterate over the ones not found and see if we can find them.
  drush_log("Looking for not founds...");
  foreach($ppids as $processor_id => $apikey) {
    \Stripe\Stripe::setApiKey($apikey);

    foreach($not_found as $customer_id => $values) {

      $not_found_in_processor_ids = $values['processor_ids'];
      $email = $values['email'];
      if (!in_array($processor_id, $not_found_in_processor_ids)) {
        try {
          $cust = \Stripe\Customer::retrieve($customer_id);
          // We found it! Make sure there isn't already one in there.
          $sql = "SELECT id FROM civicrm_stripe_customers WHERE processor_id = $processor_id AND email = '$email'";
          $dao = CRM_Core_DAO::executeQuery($sql);
          if ($dao->N == 0) {
            $sql = "UPDATE civicrm_stripe_customers SET processor_id = $processor_id WHERE id = '$customer_id';";
          }
          else {
            // We already have this email with this processor_id, we can simply delete this extra one.
            $sql = "DELETE from civicrm_stripe_customers WHERE id = '$customer_id'";
          }
          echo "$sql\n";
        }
        catch (Stripe\Error\InvalidRequest $e) {
          continue; 
        }     
      }
    }
  }
}
/*
 * If a stripe subscription doesn't match anything in the civicrm_stripe_subscription table
 * we may be able to fix it. You first have to use the Stripe.ListEvents api to find all the
 * events that have not been procssed. Then create a text file with one line for each failure
 * in the form:
 * sub:cus
 * Then feed that file to this command.
 */
function drush_stripe_repair_subs() {
  _civicrm_init();
  $file = drush_get_option('file', NULL);
  $ppid = drush_get_option('ppid', NULL);
  if (!file_exists($file)) {
    drush_log(dt("Please pass path to file with colon separated sub:cust codes. Can't find '@file'.", array('@file' => $file)), 'error');
    return;
  }

  if (empty($ppid)) {
    $sql = "SELECT pp.id FROM civicrm_payment_processor pp JOIN 
      civicrm_payment_processor_type ppt 
      ON pp.payment_processor_type_id = ppt.id 
      WHERE ppt.name = 'Stripe' AND pp.is_test = 0 
      AND pp.is_active = 1 AND ppt.is_active = 1";
    $dao = CRM_Core_DAO::executeQuery($sql);
    if ($dao->N > 1) {
      drush_log(dt("More than one Stripe payment processor, please pass ppid."), 'error');
      return;
    }
    $dao->fetch();
    $ppid = $dao->id; 
  }

  $pairs = file($file);
  foreach($pairs as $line) {
    $parts = explode(':', $line);
    $sub = trim($parts[0]);
    $cus = trim($parts[1]);

    $sql = "SELECT cr.id, contribution_status_id, cr.contact_id FROM civicrm_stripe_customers sc JOIN civicrm_email e ON sc.email = e.email ".
      "JOIN civicrm_contribution_recur cr ON cr.contact_id = e.contact_id WHERE sc.id = %0";
    $params = array(0 => array($cus, 'String'));
    $dao = CRM_Core_DAO::executeQuery($sql, $params);
    if ($dao->N == 0) {
      // If we can't find the customer or related recurring contribution, we are in trouble.
      drush_log(dt("Can't find customer (@cus) for subscription (@sub).", array('@cus' => $cus, '@sub' => $sub)), 'error');
      continue;
    }
    if ($dao->N != 1) {
      // If we have more than on record, we should stop also. 
      drush_log(dt("Found more than one record for customer (@cus) for subscription (@sub).", array('@cus' => $cus, '@sub' => $sub)), 'error');
      continue;
    }

    $dao->fetch();
    $contribution_recur_id = $dao->id;
    $contact_id = $dao->contact_id;
    $contribution_status_id = $dao->contribution_status_id;

    // We expect that this subscription is not in the civicrm_stripe_subscription table but let's be sure.
    $sql = "SELECT subscription_id FROM civicrm_stripe_subscriptions WHERE subscription_id = %0";
    $params = array(0 => array($sub, 'String'));
    $dao = CRM_Core_DAO::executeQuery($sql, $params);
    if ($dao->N != 0) {
      drush_log(dt("Found subscription (@sub) in civicrm_stripe_subscriptions.", array('@sub' => $sub)), 'error');
      continue;
    }

    // Now make sure this contribution id is not already in there...
    $sql = "SELECT subscription_id FROM civicrm_stripe_subscriptions WHERE contribution_recur_id = %0";
    $params = array(0 => array($contribution_recur_id, 'Integer'));
    $dao = CRM_Core_DAO::executeQuery($sql, $params);
    if ($dao->N != 0) {
      drush_log(dt("Found contribution_recur_id (@id) in civicrm_stripe_subscriptions.", array('@id' => $contribution_recur_id)), 'error');
      continue;
    }

    // Now, insert it.
    $sql = "INSERT INTO civicrm_stripe_subscriptions SET subscription_id = %0, customer_id = %1, contribution_recur_id = %2, is_live = 1, processor_id = %3";
    $params = array(
      0 => array($sub, 'String'),
      1 => array($cus, 'String'),
      2 => array($contribution_recur_id, 'Integer'),
      3 => array($ppid, 'Integer')
    );
    drush_log(dt("Success for @sub and @cus.", array('@sub' => $sub, '@cus' => $cus)), 'ok');
    CRM_Core_DAO::executeQuery($sql, $params);

    if ($contribution_status_id != 5) {
      drush_log(dt("Contribution status id is incorrect - may need to manually tweak."), 'error');
    }
  }

}
function drush_stripe_sync_plans() {
  _civicrm_init();

  // Get all payment processors.
  $sql = "SELECT id, user_name FROM civicrm_payment_processor WHERE is_active = 1 AND ".
      "is_test = 0 AND url_site LIKE '%api.stripe.com%'";
  $dao = CRM_Core_DAO::executeQuery($sql);
  while ($dao->fetch()) {
    $ppid = $dao->id;
    $user_name = $dao->user_name;
    drush_log("Working on payment processor $ppid", 'ok');
    \Stripe\Stripe::setApiKey($user_name);

    // First ensure all plans in CiviCRM are also in Stripe. If a plan is
    // in CiviCRM and not in stripe, delete it from CiviCRM so it will be
    // created. 
    $sql = "SELECT plan_id FROM civicrm_stripe_plans WHERE is_live = 1 AND processor_id = %0";
    $params = array(0 => array($ppid, 'Integer'));
    $plan_dao = CRM_Core_DAO::executeQuery($sql, $params);
    while($plan_dao->fetch()) {
      $plan_id = $plan_dao->plan_id;
      drush_log("Working on plan: $plan_id", 'ok');
      try {
        // Try to get this plan from Stripe.
        $plan = \Stripe\Plan::retrieve($plan_id);
        drush_log("Exists: no action taken.", 'ok');
      }
      catch ( Stripe\Error\InvalidRequest $e ) {
        //echo $e->getJsonBody()['error']['message'] . "\n";
        drush_log("MISSING: $plan_id, deleting it", 'ok');
        // Delete it from the civicrm_stripe_plans table so it will be
        // re-created. 
        $sql = "DELETE FROM civicrm_stripe_plans WHERE plan_id = %0 AND processor_id = %1";
        $params = array(0 => array($plan_id, 'String'), 1 => array($ppid, 'Integer'));
        CRM_Core_DAO::executeQuery($sql, $params);
      }
    }

    // Now go in the other direction. Ensure all plans in Stripe are in CiviCRM.
    // If one is missing, create it in CiviCRM.
    drush_log("Getting plans from stripe.");
    $plans = \Stripe\Plan::all(array('limit' => 100));
    foreach ($plans['data'] as $plan) {
      drush_log("Got plan from Stripe - " . $plan->id, 'ok');
      // See if this plan exists.
      $sql = "SELECT plan_id FROM civicrm_stripe_plans WHERE plan_id = %0 AND processor_id = %1";
      $params = array(0 => array($plan->id, 'String'), 1 => array($ppid, 'Integer'));
      $plan_dao = CRM_Core_DAO::executeQuery($sql, $params);
      if ($plan_dao->N == 0) {
        drush_log("Plan doesn't exist, creating it.");
        // Doesn't exist, create it.
        $sql = "INSERT INTO civicrm_stripe_plans SET plan_id = %0, is_live = 1, processor_id = %1";
        // Use same params as search.
        CRM_Core_DAO::executeQuery($sql, $params);
      }
      else {
        drush_log("Plan exists, no action.");
      }
    }
  }
}
