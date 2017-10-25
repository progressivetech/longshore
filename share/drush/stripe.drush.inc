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
  return $items;
}

function drush_stripe_sync_plans() {
  _civicrm_init();

  // Include Stripe library.
  require_once('packages/stripe-php/init.php');

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
