<?php

error_reporting(LONG_ERROR_REPORTING);
global $civicrm_setting;
$civicrm_setting["Extension Preferences"]["ext_repo_url"] = FALSE;
$civicrm_setting["CiviCRM Preferences"]["communityMessagesUrl"] = FALSE;
$civicrm_setting["Geocoding Preferences"]["yahoo_boss_api_key"] = "LONG_YAHOO_BOSS_API_KEY";
$civicrm_setting["Geocoding Preferences"]["yahoo_boss_api_secret"] = "LONG_BOSS_API_SECRET";
$civicrm_setting["Geocoding Preferences"]["path_to_oauth_library"] = "/usr/share/php/OAuth.php";

// All sites have access to extra language fonts.
define("K_PATH_FONTS", "/var/www/powerbase/sites/all/libraries/fonts/");
$civicrm_setting["CiviCRM Preferences"]["additional_fonts"] = array(
  "batang" => "Batang",
  "dotum" => "Dotum",
  "hline" => "Hline",
  "gulim" => "Gulim",
);

