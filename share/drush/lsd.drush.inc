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

  $items['lsd-setup'] = [
    'description' => 'Ensure Drupal settings critical to basic functionality are properly set.',
  ];
  $items['lsd-initialize'] = [
    'description' => 'Set drupal initial settings on new sites that maybe overriden later.',
  ];
  $items['lsd-set-perms'] = [
    'description' => 'Initialize default permissions.',
  ];
  $items['lsd-dev'] = [
    'description' => 'Configure drupal to operate in dev mode.',
  ];
  $items['lsd-prepare-theme'] = [
    'description' => 'Configure a new theme.',
    'options' => [
      'name' => "The name of the theme.",
      'default' => "Set this theme as the default theme.",
    ],
  ];
  $items['lsd-fix-missing'] =[
    'description' => "Delete modules and themes that were installed but no longer are present."
  ];

  $items['lsd-update-report'] =
    array('description' => "Print out report of modules and themes that need to be updated."
  );

  $items['lsd-prepare-webform-migration'] = [
    'description' => "Update webform component table to avoid errors when migrating to Drupal 8+",
  ];

  $items['lsd-admin-password'] = [
    'description' => "Set admin password to database password.",
  ];
  return $items;
}

/**
 * Implementation of command lsd-set-perms
 *
 * Initialize default Drupal permissions. If a group
 * screws up their permissions, run this command to re-initialize
 * them.
 */
function drush_lsd_set_perms() {
  lsd_set_anonymous_and_authenticated_perms();
  lsd_set_powerbase_roles_and_perms();
  // workaround for https://drupal.org/node/1454686
  // and https://drupal.org/node/1153072
  lsd_grant_admin_all_perms();
}

/**
 * Implementation of command lsd-dev.
 *
 * Settings useful for debugging in drupal. See corresponding
 * CiviCRM Api: PowerbaseSetup.Dev.
 *
 */
function drush_lsd_dev() {
  $modules = array('devel');
  module_enable($modules);

  // Allow anon users to see dsm calls.
  user_role_grant_permissions(1, array('access devel information'));

  // Allow admin role access to sql tab in reports.
  user_role_grant_permissions(3, array('view report sql'));

  // Make it easier to debug css and js issues.
  variable_set('preprocess_css', 0);
  variable_set('preprocess_js', 0);

  // The db password will have changed when moving a database, so update the
  // admin one to match.
  lsd_set_admin_password(); 
}

function drush_lsd_admin_password() {
  lsd_set_admin_password(); 
}

/**
 * Implementation of command lsd-setup.
 *
 * Ensure all essential drupal settings are set
 */
function drush_lsd_setup() {
  $settings = array(
    'file_public_path' => 'sites/default/files',
    'file_private_path' => 'sites/default/private',
    'file_temporary_path' => 'sites/default/private/temp'
  );
  foreach($settings as $k => $v) {
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

  // Having different timezones can cause serious confusion so it should only
  // be enabled on sites with staff in different time zones.
  variable_set('configurable_timezones', 0);

  // Set the default civicrm admin theme.
  variable_set('civicrmtheme_theme_admin', 'shoreditch_companion_d7_theme');

  // Set a few global theme defaults.
  $theme_settings = variable_get('theme_settings');
  $theme_settings['favicon_path'] = 'sites/all/extensions/setup/images/poweron.ico';
  $theme_settings['default_favicon'] = 0;
  variable_set('theme_settings', $theme_settings);

  // Even if we don't make shoreditch the default, we will prepare it to be made
  // the default by removing all blocks except main.
  $fields = array(
    'status' => 0,
    'region' => -1
  );
  db_update('block')
    ->fields($fields)
    ->condition('theme', 'shoreditch_companion_d7_theme', '=')
    ->condition('delta', 'main', '<>')
    ->execute();

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

  variable_set('site_default_country','US');
  variable_set('admin_theme', '0');
  variable_set('node_admin_theme', 0);

  lsd_disable_webform_promote_to_front();
  lsd_set_admin_password();
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
  foreach ($all_permissions as $perm) {
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
      ':region' => $regions['left_sidebar'],
      ':status' => 1,
      ':module' => 'system',
      ':delta' => 'help',
      ':weight' => '-9',
      ':title' => '',

    ),
    9 => array(
      ':region' => $regions['content'],
      ':status' => 1,
      ':module' => 'system',
      ':delta' => 'main',
      ':weight' => '-9',
      ':title' => '',

    ),
    10 => array(
      ':region' => $regions['disabled'],
      ':status' => 0,
      ':module' => 'system',
      ':delta' => 'main-menu',
      ':weight' => '-6',
      ':title' => '',

    ),
    11 => array(
      ':region' => $regions['left_sidebar'],
      ':status' => 1,
      ':module' => 'system',
      ':delta' => 'management',
      ':weight' => '-11',
      ':title' => '',

    ),
    12 => array(
      ':region' => $regions['disabled'],
      ':status' => 0,
      ':module' => 'system',
      ':delta' => 'navigation',
      ':weight' => '-5',
      ':title' => '',

    ),
    13 => array(
      ':region' => $regions['disabled'],
      ':status' => 0,
      ':module' => 'system',
      ':delta' => 'powered-by',
      ':weight' => '-4',
      ':title' => '',

    ),
    14 => array(
      ':region' => $regions['disabled'],
      ':status' => 0,
      ':module' => 'system',
      ':delta' => 'user-menu',
      ':weight' => '1',
      ':title' => '',

    ),
    15 => array(
      ':region' => $regions['left_sidebar'],
      ':status' => 1,
      ':module' => 'user',
      ':delta' => 'login',
      ':weight' => '-8',
      ':title' => '',

    ),
    16 => array(
      ':region' => $regions['disabled'],
      ':status' => 0,
      ':module' => 'user',
      ':delta' => 'new',
      ':weight' => '2',
      ':title' => '',

    ),
    17 => array(
      ':region' => $regions['disabled'],
      ':status' => 0,
      ':module' => 'user',
      ':delta' => 'online',
      ':weight' => '3',
      ':title' => '',

    ),
    18 => array(
      ':region' => $regions['left_sidebar'],
      ':status' => 1,
      ':module' => 'locale',
      ':delta' => 'language',
      ':weight' => '-11',
      ':title' => '',
    ),
  );

  foreach ($blocks as $block) {
    // Add theme to the block array.
    $block[':theme'] = $theme;

    // Check for a matching block that exists in the database.
    $sql = "SELECT bid FROM {block} WHERE theme = :theme AND ".
      "module = :module AND delta = :delta";
    
    $params = $block;
    // unset unused params for select query
    unset($params[':region']);
    unset($params[':status']);
    unset($params[':weight']);
    unset($params[':title']);
    $result = db_query($sql, $params);
    $record = $result->fetch();

    // Update the record with the new values.
    $fields = [];
    foreach ($block as $k => $v) {
      $field = trim($k,':');
      $fields[$field] = $v;
    }
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
    // db_query($sql, $params);
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

function lsd_set_powerbase_roles_and_perms() {
  $roles = array(
    'Powerbase Administrator',
    'Powerbase User'
  );
  // Get an array of all civi defined permissions
  // Note: civicrm_permission() is a civi function
  // defined in civicrm.module.
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
  foreach ($user_exceptions as $exception) {
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
  
  // View user profiles and change username
  $perms['Powerbase Administrator'][] = 'access user profiles';
  $perms['Powerbase Administrator'][] = 'change own username';

  foreach ($roles as $role) {
    $role_obj = user_role_load_by_name($role);
    if(!$role_obj) {
      // add the role
      $role_obj = new stdClass();
      $role_obj->name = $role;
      user_role_save($role_obj);
    }
    user_role_grant_permissions($role_obj->rid, $perms[$role]);
  }
}

function lsd_enable_modules() {
  $modules_wanted = array(
    'opb',
    'ctools',
    'webform_civicrm', 
    'file',
    'webform',
    'options_element',
    'libraries', 
    'autologout', 
    'themekey_civicrm',
    'themekey_ui', 
    'jquery_update', 
    'path',
    'civicrmtheme',
  );

  drush_log(dt("Checking which modules are already enabled."));

  $modules_already_enabled = pbd_module_list();
  $modules_to_enable = array();
  foreach ($modules_wanted as $module) {
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
  foreach ($disable as $module) {
    if(in_array($module, $modules_enabled)) {
      $modules_to_disable[] = $module;
    }
  }
  if(count($modules_to_disable) > 0) {
    drush_log(dt("Disabling unneeded modules."));
    module_disable($modules_to_disable);
  }
}

function lsd_set_anonymous_and_authenticated_perms() {
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
  foreach ($problem_anon_perms as $perm) {
    $key = array_search($perm, $default_anon_perms);
    if($key || $key == 0) unset($default_anon_perms[$key]);
  }
  $problem_auth_perms = array_diff($default_auth_perms,$available_perms);
  $key = null;
  foreach ($problem_auth_perm as $perm) {
    $key = array_search($perm, $default_auth_perms);
    if($key || $key == 0) unset($default_auth_perms[$key]);
  }
  $role_obj = user_role_load_by_name('anonymous user');
  user_role_grant_permissions($role_obj->rid,$default_anon_perms);
  $role_obj = user_role_load_by_name('authenticated user');
  user_role_grant_permissions($role_obj->rid,$default_auth_perms);
}

/**
 * Prepare a new theme.
 *
 * When creating a new theme, a number of steps have to be taken to enable
 * it to work properly as a custom theme.
 */
function drush_lsd_prepare_theme() {
  // Clear the cache so the theme is visible.
  drupal_theme_rebuild();
  $themes = system_rebuild_theme_data();
  $name = drush_get_option('name');
  // Pass --default if you want this theme to replace the default theme.'
  // This is only useful on new sites.
  $default = drush_get_option('default');
  // Ensure the theme is here (e.g. typo)
  if(!array_key_exists($name, $themes)) {
    drush_log("That theme does not exist in the filesystem.", 'error');
    return;
  }
  // Enable it.
  theme_enable(array($name));
  // Save theme settings.
  $settings = [
    'toggle_logo' => 1,
    'toggle_name' => 0,
    'toggle_slogan' => 0,
    'toggle_node_user_picture' => 0,
    'toggle_comment_user_verification' => 0,
    'toggle_favicon' => 0,
    'toggle_main_menu' => 0,
    'toggle_secondary_menu' => 0,
    'default_logo' => 0,
    'logo_path' => "sites/default/themes/${name}/${name}-logo.png",
    'bootstrap_popover_enabled' => 0,
    'bootstrap_tooltip_enabled' => 0,
  ];
  variable_set('theme_' . $name . '_settings', $settings);

  // Now let's kill the blocks.
  db_update("block")
    ->fields(array('status' => 0, 'region' => -1))
    ->condition('theme', $name, '=')
    ->condition('region', 'content', '!=')
    ->execute();

  if ($default) {
    // Replace the "default" theme with this one.
    db_update("themekey_properties")
      ->fields(array('theme' => $name))
      ->condition('theme', 'default', '=')
      ->execute();
  }
}

/**
 * Ensure the admin Drupal user's password is the same as db password.
 */
function lsd_set_admin_password() {
  require_once('includes/password.inc');
  // Get the currently set database password.
  $pass = $GLOBALS['databases']['default']['default']['password'];

  // Get the hashed password in the database.
  $sql = "SELECT pass FROM {users} WHERE uid = 1";
  $db_hashed_pass = db_query($sql)->fetch()->pass;

  if(empty($db_hashed_pass)) {
    // If we are initializing this password, we need a salt
    $db_hashed_pass = _password_generate_salt(7);
  }

  // Hash what the password should be.
  $hashed_pass = _password_crypt('sha512', $pass, $db_hashed_pass);

  // Compare...
  if($hashed_pass != $db_hashed_pass) {
    drush_log(dt("Updating Drupal admin password."));
    // Update
    db_update('users')
      ->fields(array(
        'pass' => $hashed_pass
      ))
      ->condition('uid', 1, '=')
      ->execute();
  }
  else {
    drush_log(dt("Drupal admin password already up-to-date."));
  }
}

/**
 * Command hook: fix missing modules and themes.
 */
function drush_lsd_fix_missing() {
  $sql = "SELECT name FROM system WHERE type = 'module'";
  $result = db_query($sql);
  while($name = $result->fetch()->name) {
    $filename = NULL;
    $show_error = FALSE;
    if(!drupal_get_filename('module', $name, $filename, $show_error)) {
      $sql = "DELETE FROM system WHERE name = :name";
      db_query($sql, array(':name' => $name));
    }
  }
}

/**
 * Command hook: report on modules/themes needing updates.
 */
function drush_lsd_update_report() {
  module_enable(array('update'));
  update_refresh();
  $updates_needed = array();
  if (!$available = update_get_available(TRUE)) {
    drush_log("No update data.", 'error');
  }
  else {
    module_load_include('inc', 'update', 'update.compare');
    $data = update_calculate_project_data($available);
    foreach($data as $project) {
      $recommended = NULL;
      if(array_key_exists('recommended', $project) ) {
        $recommended = $project['recommended'];
      }
      $versions = "(" .  $project['existing_version'] . " => " .
        $recommended . ")";
      $name = $project['name'];
      switch ($project['status']) {
        case UPDATE_UNKNOWN:
        case UPDATE_FETCH_PENDING:
        case UPDATE_NOT_FETCHED:
        case UPDATE_NOT_CHECKED:
          if($name != 'civicrm') {
            $updates_needed[$name] = "Didn't get update";
          }
          break;
        case UPDATE_NOT_SECURE:
          $updates_needed[$name] = "Security Update Needed ($versions)";
          break;
        case UPDATE_REVOKED:
        case UPDATE_NOT_SUPPORTED:
          break;
          $updates_needed[$name] = "No longer supported";
        case UPDATE_MAX_FETCH_ATTEMPTS:
          $updates_needed[$name] = "Too many update requests";
          break;
        case UPDATE_NOT_CURRENT:
          $updates_needed[$name] = "Update needed ($versions)";
        default:
          break;
      }
    }
  }
  module_disable(array('update'));
  print_r($updates_needed);
}

function drush_lsd_prepare_webform_migration() {
  $sql = "SELECT nid, cid, form_key 
    FROM webform_component 
    WHERE 
      type = 'fieldset' AND 
      form_key NOT LIKE 'fieldset%' AND 
      form_key NOT LIKE 'civicrm%'
  ";
  $result = db_query($sql);
  while ($record = $result->fetch()) {
    $sql = "UPDATE webform_component SET form_key = :form_key
      WHERE nid = :nid AND cid = :cid";
    $params = [ 
      ':form_key' => 'fieldset_' . $record->form_key,
      ':nid' => $record->nid,
      ':cid' => $record->cid,
    ];
    db_query($sql, $params);
  }    
}

