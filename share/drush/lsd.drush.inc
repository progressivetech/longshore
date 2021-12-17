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

  $items['lsd-setup'] = array(
    'description' => 'Ensure Drupal settings critical to basic functionality are properly set.',
  );
  $items['lsd-initialize'] = array(
    'description' => 'Set drupal initial settings on new sites that maybe overriden later.',
  );
  $items['lsd-configure'] = array(
    'description' => 'Set drupal settings common to all sites that should not be changed.',
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
 * Implementation of command lsd-setup.
 *
 * Ensure all basic drupal settings are set
 */
function drush_lsd_setup() {
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
 * Implementation of command lsd-initialize
 *
 */
function drush_lsd_initialize() {
  $force = drush_get_option('force', null);
  // should only be run once because it contains settings
  // that a group may want to change and that we don't
  // want to override.
  drush_log(dt("performing one-time configuration."), 'notice');
  if(1 == variable_get('one-time-configuration-set')) {
    if(!is_null($force)) {
      drush_log(dt("forcing one-time configuration even though it has already been set."), 'notice');
    }
    else {
      drush_log(dt("one-time configuration has already been done, skipping."), 'notice');
      return TRUE;
    }
  }

  lsd_disable_modules();
  lsd_enable_modules();
  lsd_configure_themekey();

  # All new sites get the newsflash theme.
  variable_set('theme_default','newsflash');
  variable_set('site_frontpage','civicrm');

  // Only allow user creation by admin
  variable_set('user_register',0);

  $autologout_timeout = 7200;
  $autologout_redirect_url = 'civicrm';
  variable_set('autologout_timeout', $autologout_timeout);
  variable_set('autologout_redirect_url', $autologout_redirect_url);

  variable_set('one-time-configuration-set', 1);
}

/**
 * Implementation of command lsd-configure.
 *
 */
function drush_lsd_configure() {
  // These permissions should not be changed by users and
  // if they are changed, they should be changed back.
  lsd_grant_anonymous_and_authenticated_role_access();
  lsd_create_drupal_roles();

  // jquery settings.
  variable_set('jquery_update_compression_type', 'min');
  variable_set('jquery_update_jquery_admin_version', '');
  variable_set('jquery_update_jquery_cdn', 'none');
  variable_set('jquery_update_jquery_version', '1.10');

  // Make our pages load faster. These get changed when
  // we are debugging.
  variable_set('preprocess_css',1);
  variable_set('preprocess_js',1);
  variable_set('cache',0);
  variable_set('block_cache',1);
  variable_set('page_compression',1);

  // Don't notify users when they are unblocked or activated
  // (you can choose to notify them when you create their account,
  // which is the only way to create an account anyway...).
  variable_set('user_mail_status_activated_notify', 0);

  // Handle themes for all versions.
  theme_enable(array('newsflash', 'iframe', 'seven', 'shoreditch_companion_d7_theme'));
  lsd_set_iframe_settings();
  lsd_set_newsflash_settings();
  lsd_set_blocks_for_theme('newsflash');
  lsd_set_blocks_for_theme('iframe');

  // workaround for https://drupal.org/node/1454686
  // and https://drupal.org/node/1153072
  lsd_grant_admin_all_perms();

  variable_set('site_default_country','US');
  variable_set('admin_theme', '0');
  variable_set('node_admin_theme', 0);

  lsd_disable_webform_promote_to_front();

}

function lsd_disable_webform_promote_to_front() {
  $var = variable_get('node_options_webform');
  if($var) {
    $id = array_search('promote', $var);
    if($id !== FALSE) {
      unset($var[$id]);
      variable_set('node_options_webform', $var);
    }
  }
}
function lsd_grant_admin_all_perms() {
  // heavy handed approach to compensate for a drupal core bug:
  // https://drupal.org/node/1454686
  // https://drupal.org/node/1153072

  // ensure the admin role is created and assigned
  if (!$rid = variable_get('user_admin_role')) {
    // this should be set
    $roles = user_roles();

    $role_name = 'administrator';
    $rid = array_search($role_name, $roles);
    if(empty($rid)) {
      // create an administrator role (could be new site)
      $role_obj = new stdClass();
      $role_obj->name = $role_name;
      user_role_save($role_obj);
      $role_obj = user_role_load_by_name($role_name);
      $rid = $role_obj->rid;
    }
  }

  // Create list of permissions that not even admins should have
  // access to on a production site.
  $exceptions = array('view report sql');
  // ensure the admin role has all avaialble permissions
  // $all_permissions = drupal_map_assoc(array_keys(module_invoke_all('permission')));
  $all_permissions = array_keys(module_invoke_all('permission'));
  $admin_permissions = array();
  while(list(,$perm) = each($all_permissions)) {
    if (in_array($perm, $exceptions)) {
      continue;
    }
    $admin_permissions[$perm] = true;
  }
  user_role_change_permissions($rid, $admin_permissions);
}
function lsd_set_blocks_for_theme($theme) {
  // Set default names for regions. Note: in the database, setting the
  // region to -1 disables it.
  $regions = array(
    'left_sidebar' => 'left_sidebar',
    'disabled' => -1,
    'footer' => 'footer',
    'content' => 'content'
  );

  // Now, override for specific theme names
  // for the same regions

  // Newsflash region names are a little different
  // then the generic ones.
  if($theme == 'newsflash') {
    $regions['left_sidebar'] = 'sidebar_first';
  }

  // The iframe theme should have all blocks disabled
  // except the content block. Otherwise, it's the same
  // as newsflash (because it's a sub theme)
  if($theme == 'iframe') {
    $regions['left_sidebar'] = -1;
    $regions['footer'] = -1;
  }

  // responsive_bartik / powerbase
  if($theme == 'powerbase') {
    $regions['left_sidebar'] = 'sidebar_first';
  }

  // Iterate over every standard block and define
  // how it should be displayed.
  $blocks = array(
    0 => array(
      ':region' => $regions['left_sidebar'],
      ':status' => 1,
      ':module' => 'system',
      ':delta' => 'management',
      ':weight' => '-10',
      ':title' => '',
    ),
    1 => array(
      ':region' => $regions['left_sidebar'],
      ':status' => 1,
      ':module' => 'civicrm',
      ':delta' => 1,
      ':weight' => '-14',
      ':title' => '',
    ),
    2 => array(
      ':region' => $regions['left_sidebar'],
      ':status' => 1,
      ':module' => 'civicrm',
      ':delta' => 2,
      ':weight' => '-13',
      ':title' => '',
    ),
    3 => array(
      ':region' => $regions['disabled'],
      ':status' => 0,
      ':module' => 'civicrm',
      ':delta' => 3,
      ':weight' => '-12',
      ':title' => '',
    ),
    4 => array(
      ':region' => $regions['disabled'],
      ':status' => 0,
      ':module' => 'civicrm',
      ':delta' => 4,
      ':weight' => '-12',
      ':title' => '',

    ),
    5 => array(
      ':region' => $regions['disabled'],
      ':status' => 0,
      ':module' => 'civicrm',
      ':delta' => 5,
      ':weight' => '-10',
      ':title' => '',

    ),
    6 => array(
      ':region' => $regions['disabled'],
      ':status' => 0,
      ':module' => 'civicrm',
      ':delta' => 1,
      ':weight' => '-9',
      ':title' => '',

    ),
    7 => array(
      ':region' => $regions['disabled'],
      ':status' => 0,
      ':module' => 'civicrm',
      ':delta' => 1,
      ':weight' => '-11',
      ':title' => '',

    ),
    8 => array(
      ':region' => $regions['footer'],
      ':status' => 1,
      ':module' => 'opb',
      ':delta' => 'bestviewed',
      ':weight' => '-13',
      ':title' => '',

    ),
    9 => array(
      ':region' => $regions['left_sidebar'],
      ':status' => 1,
      ':module' => 'system',
      ':delta' => 'help',
      ':weight' => '-9',
      ':title' => '',

    ),
    10 => array(
      ':region' => $regions['content'],
      ':status' => 1,
      ':module' => 'system',
      ':delta' => 'main',
      ':weight' => '-9',
      ':title' => '',

    ),
    11 => array(
      ':region' => $regions['disabled'],
      ':status' => 0,
      ':module' => 'system',
      ':delta' => 'main-menu',
      ':weight' => '-6',
      ':title' => '',

    ),
    12 => array(
      ':region' => $regions['left_sidebar'],
      ':status' => 1,
      ':module' => 'system',
      ':delta' => 'management',
      ':weight' => '-11',
      ':title' => '',

    ),
    13 => array(
      ':region' => $regions['disabled'],
      ':status' => 0,
      ':module' => 'system',
      ':delta' => 'navigation',
      ':weight' => '-5',
      ':title' => '',

    ),
    14 => array(
      ':region' => $regions['disabled'],
      ':status' => 0,
      ':module' => 'system',
      ':delta' => 'powered-by',
      ':weight' => '-4',
      ':title' => '',

    ),
    15 => array(
      ':region' => $regions['disabled'],
      ':status' => 0,
      ':module' => 'system',
      ':delta' => 'user-menu',
      ':weight' => '1',
      ':title' => '',

    ),
    16 => array(
      ':region' => $regions['left_sidebar'],
      ':status' => 1,
      ':module' => 'user',
      ':delta' => 'login',
      ':weight' => '-8',
      ':title' => '',

    ),
    17 => array(
      ':region' => $regions['disabled'],
      ':status' => 0,
      ':module' => 'user',
      ':delta' => 'new',
      ':weight' => '2',
      ':title' => '',

    ),
    18 => array(
      ':region' => $regions['disabled'],
      ':status' => 0,
      ':module' => 'user',
      ':delta' => 'online',
      ':weight' => '3',
      ':title' => '',

    ),
    19 => array(
      ':region' => $regions['left_sidebar'],
      ':status' => 1,
      ':module' => 'opb',
      ':delta' => 'phone_email_search',
      ':weight' => '3',
      ':title' => '',
    ),
    20 => array(
      ':region' => $regions['left_sidebar'],
      ':status' => 1,
      ':module' => 'locale',
      ':delta' => 'language',
      ':weight' => '-11',
      ':title' => '',
    ),
  );

  while(list(,$block) = each ($blocks)) {

    $sql = "SELECT bid FROM {block} WHERE theme = :theme AND ".
      "module = :module AND delta = :delta";
    // add theme
    $block[':theme'] = $theme;

    $params = $block;
    // unset unused params for select query
    unset($params[':region']);
    unset($params[':status']);
    unset($params[':weight']);
    unset($params[':title']);

    $result = db_query($sql, $params);

    reset($block);
    $fields = array();
    while(list($k,$v) = each($block)) {
      $field = trim($k,':');
      $fields[$field] = $v;
    }
    $record = $result->fetch();
    if(empty($record)) {
      $fields['pages'] = '';
      db_insert('block')
        ->fields($fields)
        ->execute();
    } else {
      db_update('block')
        ->fields($fields)
        ->condition('bid', $record->bid, '=')
        ->execute();
    }
    db_query($sql, $params);
    // Seems to be bug in some versions of drush
    if(function_exists('drush_cache_clear_block')) {
      drush_cache_clear_block();
    }
    else {
      cache_clear_all(NULL, 'cache_block');
    }
  }
}
function lsd_set_newsflash_settings() {
  $theme_newsflash_settings = variable_get('theme_newsflash_settings', array());
  // the newsflash banner is the "made by roople" thing in the footer
  $theme_newsflash_settings['newsflash_banner'] = 0;

  // these lines add our custom css stuff
  $theme_newsflash_settings['newsflash_uselocalcontent'] = 1;
  $theme_newsflash_settings['newsflash_localcontentfile'] = 'sites/all/themes/pb_custom_theme_elements/pb-newsflash.css';

  // override the default favicon
  $theme_newsflash_settings['toggle_favicon'] = 1;
  $theme_newsflash_settings['default_favicon'] = 0;
  $theme_newsflash_settings['favicon_path'] = 'sites/all/themes/pb_custom_theme_elements/poweron.ico';
  $theme_newsflash_settings['favicon_mimetype'] = 'image/vnd.microsoft.icon';

  // and override the default logo
  $theme_newsflash_settings['default_logo'] = 0;
  $theme_newsflash_settings['logo_path'] = 'sites/all/themes/pb_custom_theme_elements/poweron.png';

  // who doesn't want a breadcrumb?
  $theme_newsflash_settings['newsflash_breadcrumb'] = 1;
  variable_set('theme_newsflash_settings', $theme_newsflash_settings);
}
function lsd_set_iframe_settings() {
  $theme_iframe_settings = variable_get('theme_iframe_settings', array());
  $theme_iframe_settings['newsflash_banner'] = 0;
  $theme_iframe_settings['toggle_logo'] = 0;
  $theme_iframe_settings['toggle_name'] = 0;
  $theme_iframe_settings['toggle_favicon'] = 0;
  $theme_iframe_settings['toggle_main_menu'] = 0;
  $theme_iframe_settings['toggle_secondary_menu'] = 0;
  $theme_iframe_settings['default_logo'] = 0;
  $theme_iframe_settings['newsflash_breadcrumb'] = 0;
  $theme_iframe_settings['newsflash_width'] = '90%';
  $theme_iframe_settings['newsflash_leftsidebarwidth'] = 0;
  $theme_iframe_settings['newsflash_rightsidebarwidth'] = 0;
  $theme_iframe_settings['newsflash_logowidth'] = 0;
  $theme_iframe_settings['newsflash_logoheight'] = 0;
  $theme_iframe_settings['favicon_path'] = '';
  variable_set('theme_iframe_settings', $theme_iframe_settings);
}

function lsd_create_drupal_roles() {
  $roles = array(
    'Powerbase Administrator',
    'Powerbase User'
  );
  // get an array of all civi defined permissions

  // FIXME - need native drupal way to get this.
  $civi_perms = civicrm_permission();

  // Don't grant access to "access all custom data"
  // This perm will be granted to anon and authenticated
  // users below, which will grant everyone access. However,
  // some sites may want to remove this priv so ACLs can
  // do a more fine tuned approach.
  unset($civi_perms['access all custom data']);

  // There are some permissions that nobody should get on a production site.
  $never_access_perms = array('view report sql');
  foreach($never_access_perms as $never_access_perm) {
    unset($civi_perms[$never_access_perm]);
  }
  // Let's get admin too.
  $never_access_roles = array_merge($roles, array('administrator'));
  foreach($never_access_roles as $never_access_role) {
    if(FALSE !== ($role_obj = user_role_load_by_name($never_access_role))) {
      user_role_revoke_permissions($role_obj->rid, $never_access_perms);
    }
  }

  $civi_perms = array_keys($civi_perms);

  // Powerbase Admins  get all civi perms
  $perms['Powerbase Administrator'] = $civi_perms;

  // Powerbase Users get all with some exceptions
  $perms['Powerbase User'] = $civi_perms;
  $user_exceptions = array(
    'administer reserved groups',
    'adminster reserved tags',
    'administer dedupe rules',
    'administer reserved reports',
    'delete in CiviContribute',
    'administer payment processors',
    'all CiviCRM permissions and ACLs',
    // VSEA has a custom extension to provide a permission to access
    // the download data permission.
    'access export menu',
  );

  // Ensure they are deleted if they have been granted in the past
  if(FALSE !== ($role_obj = user_role_load_by_name('Powerbase User'))) {
    user_role_revoke_permissions($role_obj->rid, $user_exceptions);
  }
  // Remove them from the list to grant access to
  while(list(,$exception) = each($user_exceptions)) {
    if(false !== ($key = array_search($exception, $perms['Powerbase User']))) {
      unset($perms['Powerbase User'][$key]);
    }
  }

  // not sure if this is necessary but shouldn't hurt
  $perms['Powerbase Administrator'][] = 'access content';
  $perms['Powerbase User'][] = 'access content';

  // Admins should have access to the admin area
  $perms['Powerbase Administrator'][] = 'access administration pages';
  $perms['Powerbase Administrator'][] = 'access content overview';

  // Set paths
  $perms['Powerbase Administrator'][] = 'create url aliases';

  // Ability to enable modles
  $perms['Powerbase Administrator'][] = 'administer modules';

  // Admins should be able to have full control over all drupal content
  $perms['Powerbase Administrator'][] = 'administer nodes';

  // Admin should be able to modify drupal users
  $perms['Powerbase Administrator'][] = 'administer permissions';
  $perms['Powerbase Administrator'][] = 'administer users';
  $perms['Powerbase Administrator'][] = 'access administration pages';

  // Make sure Powerbase admins get civicrm_cicero privs
  if(module_exists('civicrm_cicero')) {
    $perms['Powerbase Administrator'][] = 'administer civicrm cicero';
  }

  // Full access to webform
  $perms['Powerbase Administrator'][] = 'create webform content';
  $perms['Powerbase Administrator'][] = 'edit own webform content';
  $perms['Powerbase Administrator'][] = 'edit any webform content';
  $perms['Powerbase Administrator'][] = 'delete any webform content';
  $perms['Powerbase Administrator'][] = 'delete own webform content';
  $perms['Powerbase Administrator'][] = 'access all webform results';
  $perms['Powerbase Administrator'][] = 'access own webform results';
  $perms['Powerbase Administrator'][] = 'edit all webform submissions';
  $perms['Powerbase Administrator'][] = 'delete all webform submissions';
  $perms['Powerbase Administrator'][] = 'access own webform submissions';
  $perms['Powerbase Administrator'][] = 'edit own webform submissions';
  $perms['Powerbase Administrator'][] = 'delete own webform submissions';

  // Ability to get backups
  if(module_exists('backup_migrate')) {
    $perms['Powerbase Administrator'][] = 'access backup and migrate';
    $perms['Powerbase Administrator'][] = 'perform backup';
    $perms['Powerbase Administrator'][] = 'access backup files';
    $perms['Powerbase Administrator'][] = 'delete backup files';
  }

  // Admins should have themekey admin privs
  $perms['Powerbase Administrator'][] = 'administer theme assignments';
  $perms['Powerbase Administrator'][] = 'administer themekey settings';

  // And ability to access theme area
  $perms['Powerbase Administrator'][] = 'administer themes';
  
  while(list(,$role) = each($roles)) {
    $role_obj = user_role_load_by_name($role);
    if(!$role_obj) {
      // add the role
      $role_obj = new stdClass();
      $role_obj->name = $role;
      user_role_save($role_obj);
    }
    user_role_grant_permissions($role_obj->rid,$perms[$role]);
  }
}

function lsd_enable_modules() {
  $modules_wanted = array(
    'opb',
    'ctools',
    'webform_civicrm', 'file',
    'webform','options_element',
    'libraries', 'autologout', 'themekey_civicrm',
    'themekey_ui', 'jquery_update', 'path'
  );

  drush_log(dt("Checking which modules are already enabled."));

  $modules_already_enabled = pbd_module_list();
  $modules_to_enable = array();
  while(list(,$module) = each($modules_wanted)) {
    if(!in_array($module, $modules_already_enabled)) {
      $modules_to_enable[] = $module;
    }
  }
  if(count($modules_to_enable) > 0) {
    drush_log(dt("Enabling Drupal modules."));
    module_enable($modules_to_enable);
  }
  else {
    drush_log(dt("No modules to enable."));
  }
}

function lsd_configure_themekey() {
  module_enable(['themekey']);

  // We have default settings for the major changes we often use. By default,
  // we set the theme to the default theme, making it easy to change once we
  // have a new custom theme.
  $settings = array(
    '(civicrm/profile/edit|civicrm/profile/view|civicrm/profile/create)',
    '(civicrm/mailing/subscribe|civicrm/mailing/unsubscribe|civicrm/mailing/optout|civicrm/mailing/resubscribe|civicrm/mailing/confirm)',
    '(civicrm/contribute/transact|civicrm/pcp|civicrm/contribute/campaign|civicrm/contribute/unsubscribe|civicrm/contribute/updatebilling|civicrm/contribute/updaterecur)',
    '(civicrm/petition/sign|civicrm/petition/thankyou)',
    '(civicrm/event/info|civicrm/event/list|civicrm/event/register|civicrm/event/confirm|civicrm/mailing/forward|civicrm/event/ical|civicrm/event/view_cart|event/cart_checkout)',
  );
  $weight_query = db_query("SELECT MAX(weight) AS weight FROM {themekey_properties}")->fetchObject();
  $weight = empty($weight_query->weight) ? 1 : $weight_query->weight;
  foreach($settings as $value) {
    $status = db_query("SELECT id FROM {themekey_properties} WHERE value = :value",
      array(':value' => $value))->fetchObject();
    if(empty($status->id)) {
      // Insert it
      db_insert("themekey_properties")
        ->fields(array(
          'property' => 'drupal:get_q',
          'operator' => '~',
          'value' => $value,
          'weight' => $weight,
          'theme' => 'default',
          'enabled' => 1,
          'wildcards' => serialize(array()),
          'module' => 'themekey'
        ))->execute();
    }
  }
}

function lsd_disable_modules() {
  $disable = array(
    'update', 
    'contextual',
    'dashboard',
    'file',
    'image',
    'overlay',
    'rdf',
    'search',
    'shortcut',
    'toolbar',
    'bartik',
    'seven', 
    'comment'
  );

  $modules_enabled = drush_module_list();
  $modules_to_disable = array();
  while(list(,$module) = each($disable)) {
    if(in_array($module, $modules_enabled)) {
      $modules_to_disable[] = $module;
    }
  }
  if(count($modules_to_disable) > 0) {
    drush_log(dt("Disabling unneeded modules."));
    module_disable($modules_to_disable);
  }
}

function lsd_grant_anonymous_and_authenticated_role_access() {
  $default_anon_perms = array(
    'access CiviMail subscribe/unsubscribe pages',
    'make online contributions',
    'register for events',
    'view event info',
    'view public CiviMail content',
    'sign CiviCRM Petition',
    'profile create',
    'access content',
    'access uploaded files',
    // Needed for Stripe.
    'access AJAX API',
  );

  // auth perms are same with one addition.
  $default_auth_perms = $default_anon_perms;
  $default_auth_perms[] = 'access Contact Dashboard';

  // Ensure we are not setting permissions that don't exist.
  $module_perms = user_permission_get_modules();
  $available_perms = array_keys($module_perms);

  $problem_anon_perms = array_diff($default_anon_perms,$available_perms);
  $key = null;
  while(list(,$perm) = each($problem_anon_perms)) {
    $key = array_search($perm, $default_anon_perms);
    if($key || $key == 0) unset($default_anon_perms[$key]);
  }
  $problem_auth_perms = array_diff($default_auth_perms,$available_perms);
  $key = null;
  while(list(,$perm) = each($problem_auth_perms)) {
    $key = array_search($perm, $default_auth_perms);
    if($key || $key == 0) unset($default_auth_perms[$key]);
  }
  $role_obj = user_role_load_by_name('anonymous user');
  user_role_grant_permissions($role_obj->rid,$default_anon_perms);
  $role_obj = user_role_load_by_name('authenticated user');
  user_role_grant_permissions($role_obj->rid,$default_auth_perms);
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
