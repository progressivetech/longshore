<?php

/**
 * @file 
 *
 * Provisioning functions used by longshore to setup
 * Powerbase sites.
 *
 */

/**
 * Implements hook_drush_command().
 */
function lsd_drush_command() {
  $items = array();
  $items['lsd-civicrm-settings'] = array(
    'description' => 'Generate a civicrm.settings.php file.',
    'options' => array(
      'base_url' => 'Base URL for the site, including the protocol'
    ),
  );
  $items['lsd-civicrm-install'] = array(
    'description' => 'Install civicrm database .',
    'options' => array(
      'base_url' => 'Base URL for the site, including the protocol'
    ),
  );
  return $items;
}

/**
 * Implementation of command 'lsd-civicrm-install'
 *
 * Unfortunately, drush won't let us run the civicrm-install command
 * if civicrm is not enabled.
 */
function drush_lsd_civicrm_install() {
  $db_spec = lsd_install_prep();

  // Setting lang to empty avoids having the installer try to untar the 
  // language files (which we handle via drush make).
  $lang = '';

  _civicrm_install_db($db_spec['username'], $db_spec['password'],
    $db_spec['host'], $db_spec['database'], $db_spec['modPath'], $lang);

  _civicrm_generate_settings_file($db_spec['username'], $db_spec['password'],
    $db_spec['host'], $db_spec['database'], $db_spec['modPath']);

  module_enable(array('civicrm'));
}

/**
 * Implementation of command 'lsd-civicrm-settings'
 *
 * Create the civicrm.settings.php file. Useful when we want to install
 * civicrm and we plan to import the database - so no need to create the
 * data - just need civicrm.settings.php.
 */
function drush_lsd_civicrm_settings() {
  $db_spec = lsd_install_prep();

  _civicrm_generate_settings_file($db_spec['username'], $db_spec['password'],
    $db_spec['host'], $db_spec['database'], $db_spec['modPath']);
}

/**
 * Prepare for civicrm db or settings installation.
 *
 * Loads libraries and returns database parameters used by both
 * drush_lsd_civicrm_install and drush_lsd_civicrm_settings().
 */
function lsd_install_prep() {

  // drush won't include this because the module is not installed yet.
  require_once('/var/www/html/sites/all/modules/civicrm/drupal/drush/civicrm.drush.inc');

  // We are using the same db credentials as Drupal - so we first have to
  // retrieve those values via the db_spec.
 
  // The function drush_civicrm_get_db_spec is defined in civicrm.drush.inc.
  $db_spec = drush_civicrm_get_db_spec();

  // The CiviCRM function to generate the settings file will use the globally
  // set base_url - so we should set it based on the option we received on
  // the command line.
  global $GLOBALS;
  $GLOBALS['base_url'] = drush_get_option('base_url', FALSE);

  // The function _civicrm_get_crmpath is defined in civicrm.drush.inc
  $drupalRoot = drush_get_context('DRUSH_DRUPAL_ROOT');
  $modPath    = "$drupalRoot/sites/all/modules";

  // $crmPath is used globally in the civicrm install helper.
  global $crmPath;
  $crmPath = "${modPath}/civicrm";

  // Set some include paths. This is handled in the civicrm_setup function in
  // install/civicrm.php - but that function also messes with directory 
  // permissions so I'd rather not call it.
  $pkgPath = $crmPath . DIRECTORY_SEPARATOR . 'packages';
  set_include_path(
    $crmPath . PATH_SEPARATOR . $pkgPath . PATH_SEPARATOR .
      get_include_path()
  );

  // Bah - this file is needed for one function call to write the settings file.
  $civicrmInstallerHelper = "$crmPath/install/civicrm.php";
  require_once($civicrmInstallerHelper);

  // Not really part of db_spec - but we need this value to install CiviCRM
  // so I'm tucking into the returned array.
  $db_spec['modPath']    = $modPath;

  return $db_spec;
}
