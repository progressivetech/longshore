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
  $currency = 'usd';

  // Include Stripe library & Set API credentials.
  require_once('packages/stripe-php/init.php');
  $sql = "SELECT user_name FROM civicrm_payment_processor WHERE is_active = 1 AND ".
      "is_test = 0 AND url_site LIKE '%api.stripe.com%'";
  $dao = CRM_Core_DAO::executeQuery($sql);
  $dao->fetch();
  if(empty($dao->user_name)) {
    drush_log(dt("Failed to get stripe api user name."), 'error');
    return;
  }
  \Stripe\Stripe::setApiKey($dao->user_name);

  // Get all plans in CiviCRM.
  $sql = "SELECT plan_id FROM civicrm_stripe_plans WHERE is_live = 1";
  $dao = CRM_Core_DAO::executeQuery($sql);
  while($dao->fetch()) {
    $id = $dao->plan_id;
    try {
      // Try to get this plan from Stripe.
      $plan = Stripe_Plan::retrieve($id);
      drush_log("Exists: $id", 'ok');
    }
    catch ( Stripe_InvalidRequestError $e ) {
      //echo $e->getJsonBody()['error']['message'] . "\n";
      drush_log("MISSING: $id", 'ok');
      while(1) {
        $answer = drush_prompt("Add?", 'y', TRUE);
        if($answer == 'y') {
          drush_log("Adding...", 'ok');
          $pieces = explode('-', $id);
          $amount = $pieces[3];
          $interval = $pieces[2];
          $interval_count = $pieces[1];
          $formatted_amount = '$' . number_format(($amount / 100), 2);
          $stripe_plan = array(
            'amount' => $amount,
            'interval' => $interval,
            'name' => "CiviCRM every ${interval_count} ${interval}s ${formatted_amount}",
            'currency' => $currency, 
            'id' => $id,
            'interval_count' => $interval_count
          );
          try {
            \Stripe\Plan::create($stripe_plan);
            drush_log("Created", 'ok');
          }
          catch ( Exception $e ) {
            drush_log("There was an error", 'error');
            drush_log($e->getJsonBody()['error']['message'], 'error');
          }
          break;
        }
        elseif($answer == 'n') {
          drush_log("Not entering", 'ok');
          break;
        }
      }
    }
  }
}
