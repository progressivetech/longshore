<?php

/**
 * @file
 *
 * This scripts handles one off authnet queries to the authnet api. 
 **/

function authnet_drush_command() {
  $items = array();
  $items['authnet-sync-subscriptions'] =
    array('description' => "Ensure all recurring contributions are assigned to the right authnet account.");
  return $items;
}

function drush_authnet_sync_subscriptions() {
  _civicrm_init();

  // Get all payment processors.
  $sql = "SELECT id, user_name, password, url_recur FROM civicrm_payment_processor ".
    "WHERE is_active = 1 AND is_test = 0 AND url_site LIKE ".
    "'%secure.authorize.net%'";
  $dao = CRM_Core_DAO::executeQuery($sql);
  $pps = array();
  while ($dao->fetch()) {
    $pps[$dao->id] = array(
      'user_name' => $dao->user_name, 
      'password' => $dao->password,
      'url_recur' => $dao->url_recur,
    );
  }
  $template = '<ARBGetSubscriptionRequest xmlns="AnetApi/xml/v1/schema/AnetApiSchema.xsd">
	<merchantAuthentication>
		<name>API_LOGIN_ID</name>
		<transactionKey>API_TRANSACTION_KEY</transactionKey>
	</merchantAuthentication>
	<subscriptionId>SUBSCRIPTION_ID</subscriptionId>
  </ARBGetSubscriptionRequest>';

  $find = array(
    'API_LOGIN_ID',
    'API_TRANSACTION_KEY',
    'SUBSCRIPTION_ID'
  );
  $replace = array();
  // Iterate over all recurring contributions that are pending.
  $sql = "SELECT id, processor_id, payment_processor_id FROM civicrm_contribution_recur WHERE is_test = 0 AND contribution_status_id IN (2, 5) AND processor_id IS NOT NULL";
  $dao = CRM_Core_DAO::executeQuery($sql);
  while($dao->fetch()) {
    $ppid = $dao->payment_processor_id;
    $subscription_id = $dao->processor_id;
    $contribution_recur_id = $dao->id;

    // Get a simple array with the payment processor ids of all available payment processors.
    $pps_candidates = array_keys($pps);

    // First try the payment processor that is set for this contribution.
    // Remove it from our list so we don't try it twice.
    unset($pps_candidates[$ppid]);

    // Create the XML to send to authorize.net.
    $replace = array(
      $pps[$ppid]['user_name'],
      $pps[$ppid]['password'],
      $subscription_id,
    );
    $arbXML = str_replace($find, $replace, $template);

    // Submit.
    $message = auth_submit($arbXML, $pps[$ppid]['url_recur']);

    // See if auth recognizes it.
		if ($message['text'] == 'The subscription cannot be found.') {
	    drush_log("Problem with: $contribution_recur_id ($ppid).", 'error');

      // Reset the remaining ppids and try each one.
      reset($pps_candidates);
      foreach($pps_candidates as $ppid) {
        $replace = array(
          $pps[$ppid]['user_name'],
          $pps[$ppid]['password'],
          $subscription_id,
        );
        $arbXML = str_replace($find, $replace, $template);
        $message = auth_submit($arbXML, $pps[$ppid]['url_recur']);
		    if ($message['text'] == 'The subscription cannot be found.') {
          continue;
        }
        elseif ($message['text'] == 'Successful.') {
          drush_log("Found in $ppid", 'ok');
          $sql = "UPDATE civicrm_contribution_recur SET payment_processor_id = %0 WHERE id = %1";
          $update_params = array(0 => array($ppid, 'Integer'), 1 => array($contribution_recur_id, 'Integer'));
          CRM_Core_DAO::executeQuery($sql, $update_params);
          continue 2;
        }
      } 
    }
    else {
      // drush_log("Properly found", 'ok');
      continue;
    }
    drush_log("Not found at all", 'error');
  }
}

function auth_submit($arbXML, $url) {
  $submit = curl_init($url);
  curl_setopt($submit, CURLOPT_RETURNTRANSFER, 1);
  curl_setopt($submit, CURLOPT_HTTPHEADER, array("Content-Type: text/xml"));
  curl_setopt($submit, CURLOPT_HEADER, 1);
  curl_setopt($submit, CURLOPT_POSTFIELDS, $arbXML);
  curl_setopt($submit, CURLOPT_POST, 1);
  curl_setopt($submit, CURLOPT_SSL_VERIFYPEER, Civi::settings()->get('verifySSL'));
  $response = curl_exec($submit);

  if (!$response) {
    drush_log("Woops, curl reported an error", 'error');
    drush_log(curl_error($submit), 'error');
    return FALSE;
  }

  curl_close($submit);
  $message = _parseArbReturn($response);
  return $message;

}

// Copied and pasted from Authorize.php
function _parseArbReturn($content) {
	$refId = _substring_between($content, '<refId>', '</refId>');
	$resultCode = _substring_between($content, '<resultCode>', '</resultCode>');
	$code = _substring_between($content, '<code>', '</code>');
	$text = _substring_between($content, '<text>', '</text>');
	$subscriptionId = _substring_between($content, '<subscriptionId>', '</subscriptionId>');
	return array(
		'refId' => $refId,
		'resultCode' => $resultCode,
		'code' => $code,
		'text' => $text,
		'subscriptionId' => $subscriptionId,
	);
}

/**
 * Helper function for _parseArbReturn.
 *
 * Function is from Authorize.Net sample code, and used to avoid using
 * PHP5 XML functions
 *
 * @param string $haystack
 * @param string $start
 * @param string $end
 *
 * @return bool|string
 */
function _substring_between(&$haystack, $start, $end) {
	if (strpos($haystack, $start) === FALSE || strpos($haystack, $end) === FALSE) {
		return FALSE;
	}
	else {
		$start_position = strpos($haystack, $start) + strlen($start);
		$end_position = strpos($haystack, $end);
		return substr($haystack, $start_position, $end_position - $start_position);
	}
}

