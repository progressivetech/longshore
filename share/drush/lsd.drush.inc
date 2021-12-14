<?php

/**
 * @file
 *
 * Provisioning functions used by longshore to setup Drupal/CiviCRM sites.
 * This file handles only the basic functions needed for a working Drupal
 * and CiviCRM installation.
 *
 * See also:
 *  * ptp.drush.inc: The PTP drush file ensures all sites have a PTP user
 *    entered and report statistics to the PTP site.
 *  * pbd.drush.inc: The PBD drush file configures a site to be a PowerBase
 *    with all required modules and extensions installed and all
 *    configuration options in place.
 */

//
/**
 * Implements hook_drush_command().
 */
function lsd_drush_command() {
  $items = array();

  $items['lsd-civicrm-install'] = array(
    'description' => 'Install civicrm database .',
    'options' => array(
      'base_url' => 'Base URL for the site, including the protocol'
    ),
  );
  $items['lsd-set-paths'] = array(
    'description' => 'Ensure all Drupal and CiviCRM paths and URLs are set correctly.',
  );
  $items['lsd-extensions-update'] = array(
    'description' => "Do database updates for all extensions.",
    'options' => array(
      'extensions' => 'Only check for updates if any of the given comma-separate list of extensions are enabled'
    ),
  );
  $items['lsd-unused-addons'] = array(
    'description' => "Check provided list of modules and extensions, return list that are not used on any site.",
    'options' => array(
      'addons' => 'Comma separated list of addons, with d: prefix for drupal modules and c: prefix for civi extensions.',
      'include' => 'Indicate whether you want a report of modules and extensions that are uninstalled or disabled.'
    ),
  );
  return $items;
}

/**
 * Implementation of lsd-unused-addons
 *
 **/
function drush_lsd_unused_addons() {
  _civicrm_init();
  $addons = drush_get_option('addons', '');
  $include_status = drush_get_option('include', 'uninstalled');
  if (empty($addons)) {
    drush_log("Please pass addons to checked via --addons.", 'error');
    return;
  }
  $addons = explode(',', $addons);
  $ret = []; 
  $extensions = [];
  $modules = [];
  foreach ($addons as $addon) {
    $addon_parts = explode(':', $addon);
    if ($addon_parts[0] == 'c') {
      $extensions[] = $addon_parts[1];
    }
    else {
      $modules[] = $addon_parts[1];
    }
  }
  foreach($extensions as $extension) {
    $params = [ 0 => [ $extension, 'String' ] ];
    $sql = "SELECT is_active FROM civicrm_extension WHERE full_name = %0";
    $dao = CRM_Core_DAO::executeQuery($sql, $params);
    $dao->fetch();
    if ($dao->N == 0 && $include_status == 'uninstalled') {
      // Not installed.
      $ret[] = 'c:' . $extension; 
    }
    elseif (isset($dao->is_active) && $dao->is_active == 0 && $include_status == 'disabled') {
      // disabled but needs to be uninstalled.
      $ret[] = 'c:' . $extension;
    }

  }
  foreach($modules as $module) {
    $params = [ 0 => [ $module, 'String' ] ];
    $sql = "SELECT status, schema_version FROM system WHERE type = 'module' AND name = %0";
    $dao = CRM_Core_DAO::executeQuery($sql, $params);
    $dao->fetch();
    if ($include_status == 'uninstalled' && ($dao->N == 0 || ($dao->status == 0 && $dao->schema_version == -1))) {
      // Uninstalled.
      $ret[] = 'd:' . $module;
    }
    elseif ($dao->status == 0 && $include_status == 'disabled') {
      // disabled but needs to be uninstalled.
      $ret[] = 'd:' . $module;
    }

  }

  return implode(',', $ret);
} 

/**
 * Implementation of command lsd-set-paths.
 *
 * Ensure all basic drupal and civicrm paths and urls are set
 */
function drush_lsd_set_paths() {
  // First, drupal paths.
  $settings = array(
    'file_public_path' => 'sites/default/files',
    'file_private_path' => 'sites/default/private',
    'file_temporary_path' => 'sites/default/private/temp'
  );
  while(list($k, $v) = each($settings)) {
    if(variable_get($k, NULL) != $v) {
      variable_set($k, $v);
    }
  }
}

/**
 * Run all extension database upgrades.
 */
function drush_lsd_extensions_update() {
  $extensions = drush_get_option('extensions', NULL);
  _civicrm_init();
  if ($extensions) {
    // Only check and run updates if the extensions in the restrct array
    // are enabled on this site.
    // We use addslashes to avoid funny business.
    $restrict = explode(',', addslashes($extensions));
    $clause = '("' . implode('","', $restrict) . '")';
    $sql = "SELECT COUNT(*) AS count FROM civicrm_extension WHERE is_active = 1 AND full_name IN $clause";
    drush_log("SQL: $sql", 'info');
    $dao = CRM_Core_DAO::executeQuery($sql);
    $dao->fetch();
    if ($dao->count == 0) {
      // None of the given extensions are enabled, short circuit.
      drush_log("Not running extensions update, no extensions specified are installed.", 'info');
      return;
    }
  }

  if (! CRM_Extension_Upgrades::hasPending()) {
    // Return early if no pending updates.
    drush_log("No pending updates.", 'info');
    return;
  }
  drush_log("Running updates.", 'info');
  $queue = CRM_Extension_Upgrades::createQueue();
  $runner = new CRM_Queue_Runner(array(
    'title' => ts('Database Upgrades'),
    'queue' => $queue,
    'errorMode'=> CRM_Queue_Runner::ERROR_ABORT,
    'onEnd' => NULL,
    'onEndUrl' => NULL
  ));

  CRM_Core_Error::debug_log_message('CRM_Admin_Page_ExtensionsUpgrade: Start upgrades');
  $out = $runner->runAll();
  if($out !== TRUE) {
    $out = print_r($out, TRUE);
    drush_log("Failed to run database updates on extensions.", 'error');
    drush_log($out, 'error');
  }
  // Lastly, refresh the list of extensions
  CRM_Extension_System::singleton()->getManager()->refresh();
}

/**
 * Reset the extensions cache.
 *
 * This step is necessary when moving sites if the path to the extensions
 * has changed.
 */
function lsd_extensions_freshen() {
  _civicrm_init();
  CRM_Extension_System::singleton()->getManager()->refresh();
}
