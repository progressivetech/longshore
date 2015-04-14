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
  $items['lsd-settings'] = array(
    'description' => 'Generate a civicrm.settings.php file.',
    'options' => array(
      'base_url' => 'Base URL for the site, including the protocol'
    ),
  );
  return $items;
}

/**
 * Implementation of command 'lsd-settings'
 */
function drush_lsd_settings() {

  // The function _civicrm_get_crmpath is defined in civicrm.drush.inc
  $crmpath = _civicrm_get_crmpath();
  $drupalRoot = drush_get_context('DRUSH_DRUPAL_ROOT');
  $modPath    = "$drupalRoot/$crmpath";

  // We are using the same db credentials as Drupal - so we first have to
  // retrieve those values via the db_spec.
  //
  // The function drush_civicrm_get_db_spec is defined in civicrm.drush.inc.
  $db_spec = drush_civicrm_get_db_spec();

  // The CiviCRM function to generate the settings file will use the globally
  // set base_url - so we should set it based on the option we received on
  // the command line.
  global $GLOBALS;
  $GLOBALS['base_url'] = drush_get_option('base_url', FALSE);

  // Bah - this file is needed for one function call.
  $civicrmInstallerHelper = "$modPath/civicrm/install/civicrm.php";
  require_once($civicrmInstallerHelper);

  _civicrm_generate_settings_file($db_spec['username'], $db_spec['password'],
    $db_spec['host'], $db_spec['database'], $modPath);

}

