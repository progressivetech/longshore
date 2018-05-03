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
  $items['lsd-set-paths'] = array(
    'description' => 'Ensure all Drupal and CiviCRM paths and URLs are set correctly.',
  );
  $items['lsd-extensions-update'] =
    array('description' => "Do database updates for all extensions.");

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

  //  _civicrm_install_db and _civicrm_generate_settings_file are defined
  //  in civicrm.drush.inc from the civicrm module.
  $load_generated_data = FALSE;
  _civicrm_install_db($db_spec['username'], $db_spec['password'],
    $db_spec['host'], $db_spec['database'], $db_spec['modPath'], $lang,
    $load_generated_data);

  _civicrm_generate_settings_file($db_spec['username'], $db_spec['password'],
    $db_spec['host'], $db_spec['database'], $db_spec['modPath']);

  module_enable(array('civicrm'));
}

/**
 * Implementation of command 'lsd-civicrm-settings'
 *
 * Create the civicrm.settings.php file. Useful when we don't need to
 * install civicrm because we simply imported the database.
 */
function drush_lsd_civicrm_settings() {
  $db_spec = lsd_install_prep();

  //  _civicrm_generate_settings_file is defined in civicrm.drush.inc from
  //  the civicrm module.
  _civicrm_generate_settings_file($db_spec['username'], $db_spec['password'],
    $db_spec['host'], $db_spec['database'], $db_spec['modPath']);
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

  // Now CiviCRM.
  _civicrm_init();

  // We only need to refresh the cache if we reset an extensions setting.
  $freshen_extensions_cache = FALSE;

  $abs_root_path = '/var/www/powerbase';
  $rel_site_path ='sites/default';
  $abs_site_path = $abs_root_path . '/' . $rel_site_path;

  // Fork - depending on CiviCRM version.
  $code_ver = CRM_Utils_System::version();
  if(version_compare($code_ver, '4.7', '<')) {
    // Use the old method. This should get ripped out after 4.6 upgrade.

    $rel_civicrm_files_path = "${rel_site_path}/files/civicrm";

    $keys = array(
      'imageUploadURL' => array(
        'value' => "${rel_civicrm_files_path}/persist/contribute/",
        'group_name' => 'URL Preferences',
      ),
      'userFrameworkResourceURL' => array(
        'value' => CIVICRM_UF_BASEURL . '/sites/all/modules/civicrm',
        'group_name' => 'URL Preferences',
      ),
      'uploadDir' => array(
        'value' => "${rel_civicrm_files_path}/upload",
        'group_name' => 'Directory Preferences',
      ),
      'customPHPPathDir' => array(
        'value' => "${abs_site_path}/custom-civicrm/php",
        'group_name' => 'Directory Preferences',
      ),
      'extensionsDir' => array(
        'value' => "${abs_root_path}/sites/all/extensions",
        'group_name' => 'Directory Preferences',
      ),
      'extensionsURL' => array(
        'value' => "sites/all/extensions",
        'group_name' => 'URL Preferences',
      ),
      'customTemplateDir' => array(
        'value' => "${abs_site_path}/custom-civicrm/templates",
        'group_name' => 'Directory Preferences',
      ),
    );

    while(list($k,$v) = each($keys)) {
      // Check if it exists and needs changing.
      $sql = 'SELECT id, value FROM civicrm_setting WHERE name = %1 AND group_name = %2';
      $params = array(
        1 => array($k, 'String'), 2 => array($v['group_name'], 'String')
      );
      $dao = CRM_Core_DAO::executeQuery($sql, $params);
      $dao->fetch();

      if(isset($dao->id)) {
        // Setting doesn't exist, create it.
        CRM_Core_BAO_Setting::setItem($v['value'], $v['group_name'], $k);
        if(preg_match("/^extensions/", $k)) {
          $freshen_extensions_cache = TRUE;
        }
      }
      elseif($dao->value != serialize($v['value'])) {
        // Setting is set incorrectly, fix it.
        $sql = "UPDATE civicrm_setting SET value = %0 WHERE id = %1";
        $params = array(
          0 => array(serialize($v['value']), 'String'),
          1 => array($dao->id, 'Integer')
        );
        CRM_Core_DAO::executeQuery($sql, $params);
        if(preg_match("/^extensions/", $k)) {
          $freshen_extensions_cache = TRUE;
        }
      }
    }
  }
  else {
    // Version 4.7 and above.
    // Only set for the ones in which we alter from the default.
    $want_settings = array(
      'extensionsDir' => "${abs_root_path}/sites/all/extensions",
      'extensionsURL' => "sites/all/extensions",
      'customTemplateDir' => "${abs_site_path}/custom-civicrm/templates",
      'customPHPPathDir' => "${abs_site_path}/custom-civicrm/php",
      'userFrameworkResourceURL' => CIVICRM_UF_BASEURL . '/sites/all/modules/civicrm'
    );

    // Get all settings and only update if they are different.
    try {
      $update_params = array();
      $results = civicrm_api3('Setting', 'get', array());
      $current_settings = array_pop($results['values']);
      while(list($k, $v) = each($want_settings)) {
        if(isset($current_settings[$k]) && $current_settings[$k] != $v) {
          // Update it.
          drush_log(dt("Updating @key to @value", array('@key' => $k, '@value' => $v)));
          $update_params[$k] = $v;
          if(preg_match("/^extensions/", $k)) {
            $freshen_extensions_cache = TRUE;
          }
        }
      }
      if(count($update_params) > 0) {
        civicrm_api3('Setting', 'create', $update_params);
      }
    }
    catch (CiviCRM_API3_Exception $e) {
      drush_log(dt("Failed to get or set Settings."), 'error');
      drush_log($e->getMessage(), 'error');
      return FALSE;
    }
  }

  if ($freshen_extensions_cache) {
    lsd_extensions_freshen();
  }
}

/**
 * Prepare for civicrm db or settings installation.
 *
 * Loads libraries and returns database parameters used by both
 * drush_lsd_civicrm_install and drush_lsd_civicrm_settings().
 */
function lsd_install_prep() {

  // drush won't include this because the module is not installed yet.
  require_once('/var/www/powerbase/sites/all/modules/civicrm/drupal/drush/civicrm.drush.inc');

  // We are using the same db credentials as Drupal - so we first have to
  // retrieve those values via the db_spec.

  // The function drush_civicrm_get_db_spec is defined in civicrm.drush.inc.
  $db_spec = drush_civicrm_get_db_spec();

  // The CiviCRM function to generate the settings file will use the globally
  // set base_url - so we should set it based on the option we received on
  // the command line.
  global $GLOBALS;
  $GLOBALS['base_url'] = drush_get_option('base_url', FALSE);

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

/**
 * Run all extension database upgrades.
 */
function drush_lsd_extensions_update() {
  _civicrm_init();
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
