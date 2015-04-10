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
  $items['lsd-create-settings'] = 
    array( 
      'description' => "Create both the settings.php and civicrm.settings.php files.",
      'options' => array(
        '--credentials' => 'Path to file containing credentials',
      ),
    );
  return $items;
}

/**
 * Drush command: lsd-create-settings
 */
function drush_lsd_create_settings() {
  $credentials = drush_get_options('credentials', NULL);
  if(!file_exists($credentials)) {
    drush_log(dt("Failed to find credentials file."), 'error');
    return FALSE;
  }
  require_once($credentials);
  // Look for required values
  if(empty($dbname) || empty($dbuser) || empty($dbhost) || empty($dbpass)) {
    drush_log(dt("Missing db connection parameter in credentials file."), 'error');
    return FALSE;
  }
  // Define array for creating drupal settings file
  $settings['databases'] = array(
    'values' => array(
      'default' => array(
        'default' => array(
          'drive' => 'mysql',
          'database' => $dbname,
          'username' => $dbuser,
          'password' => $dbpass,
          'host' => $dbhost
        ),
      ),
    ),
  );
  drupal_rewrite_settings($settings);
}
