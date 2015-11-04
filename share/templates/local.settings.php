<?php

// This header is here so that IE users that are accessing a contribution or event
// page via an iframe embedded in another web site, will get their cookie/session
// settings set properly even though the iframe is pointing to a different domain
// than the parent page. For more information on what these codes mean, see:
// https://support.ourpowerbase.net/ticket/1575
header('P3P:CP="IDC DSP COR ADM DEVi TAIi PSA PSD IVAi IVDi CONi HIS OUR IND CNT"');
error_reporting(LONG_ERROR_REPORTING);
global $civicrm_setting;
$civicrm_setting["Extension Preferences"]["ext_repo_url"] = FALSE;
$civicrm_setting["CiviCRM Preferences"]["communityMessagesUrl"] = FALSE;
$civicrm_setting["Geocoding Preferences"]["yahoo_boss_api_key"] = "LONG_YAHOO_BOSS_API_KEY";
$civicrm_setting["Geocoding Preferences"]["yahoo_boss_api_secret"] = "LONG_YAHOO_BOSS_API_SECRET";
$civicrm_setting["Geocoding Preferences"]["path_to_oauth_library"] = "/usr/share/php/OAuth.php";

// All sites have access to extra language fonts.
define("K_PATH_FONTS", "/var/www/powerbase/sites/all/libraries/fonts/");
$civicrm_setting["CiviCRM Preferences"]["additional_fonts"] = array(
  "batang" => "Batang",
  "dotum" => "Dotum",
  "hline" => "Hline",
  "gulim" => "Gulim",
);

// If we are using stripe - be sure to use the PHP submission method
// See: https://github.com/drastik/com.drastikbydesign.stripe/issues/113
define('STRIPE_SUBMISSION_METHOD', 'php');

// Turn off all security/update settings
$civicrm_setting["CiviCRM Preferences"]["securityUpdateAlert"] = "1";
$civicrm_setting["CiviCRM Preferences"]["versionAlert"] = "1";

