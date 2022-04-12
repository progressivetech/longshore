<?php

/**
 * @file
 *
 * This file contains commands used to install and maintain PowerBase
 * databases. PowerBase is based on CiviCRM and is maintained by the
 * Progressive Technology Project (http://progressivetech.org/).
 *
 */

function pbd_drush_command() {
  $items = array();
  $items['pbd-enable-debugging'] =
    array( 'description' => "Enable CiviCRM Debugging.");
  $items['pbd-disable-debugging'] =
    array( 'description' => "Disable CiviCRM Debugging.");
  $items['pbd-schema-version'] =
    array( 'description' => "Get CiviCRM schema version.");
  $items['pbd-code-version'] =
    array( 'description' => "Get CiviCRM code version");
  $items['pbd-obscure'] =
    array( 'description' => "Scrub all private data from the database. CAREFUL!!");
  $items['pbd-update-assessments'] =
    array( 'description' => "Update all assessments (depends on net.ourpowerbase.assess extension).");
  $items['pbd-prepare-theme'] =
    array(
      'description' => "Setup a new theme to work as custom theme (remove blocks, configure theme, etc.)",
      'options' => array(
        'name' => 'The name of the theme to prepare.',
        'default' => 'Whether to set it as the default in themekey.'
      )
    );
  $items['pbd-subscription-links'] =
    array(
      'description' => "Output the cancel, update, etc links for a given contact.",
      'options' => array(
        'id' => 'The contact id of the person you want the links for.'
      )
    );
  $items['pbd-add-missing-indices'] =
    array(
      'description' => "Find and add any missing CiviCRM indices."
    );

  $items['pbd-restore-dedupe-rules'] = 
    array(
      'description' => 'Restore dedupe rules to defaults.'
    );
  $items['pbd-rebuild-multilingual-schema'] = 
    array(
      'description' => 'If site is multi-lingual, rebuilds the schema.'
    );
  $items['pbd-undocufund-report'] = 
    array(
      'description' => 'Run custom report.',
      'options' => [
        'csv' => "Set to 1 to output in CSV format. Otherwise print_r will be used."
      ],
    );
  $items['pbd-cicero-users'] = 
    array(
      'description' => 'Check for cicero usage',
    );
  $items['pbd-petition-report'] =
    array(
      'description' => "Report on petition email user",
    );
  return $items;
}

/*
 * ****************************************************
 *
 * All drush commands are defined below.
 *
 * ****************************************************
 */

function drush_pbd_petition_report() {
  _civicrm_init();
  $urlPieces = explode('.', str_replace('https://', '', CIVICRM_UF_BASEURL));
  $site = array_shift($urlPieces);
  $report = NULL;
  if (!class_exists('CRM_Campaign_BAO_Survey')) {
    $report = 'Campaign Not Installed';
  }
  else {
    $petitions = \Civi\Api4\Survey::get()
      ->setCheckPermissions(FALSE)
      ->addWhere('is_active', '=', TRUE)
      ->addWhere('activity_type_id:name', '=', 'Petition')
      ->execute();
    $petitionsCount = $petitions->count();
    
    $sql = "SELECT is_active FROM civicrm_extension WHERE full_name = 'cc.tadpole.petitionemail'";
    $dao = CRM_Core_DAO::executeQuery($sql);
    if ($dao->N == 0) {
      $report = 'Extension Not Installed';
    }
    else {
      $dao->fetch();
      if ($dao->is_active == 1) {
        $petitionActivityTypeId = \Civi\Api4\OptionValue::get()
          ->setCheckPermissions(FALSE)
          ->addSelect('value')
          ->addWhere('name', '=', 'Petition')
          ->addWhere('option_group_id:name', '=', 'activity_type')
          ->execute()->first()['value'];
        $sql = "SELECT COUNT(*) AS count FROM civicrm_survey s
          JOIN civicrm_petition_email e ON s.id = e.petition_id 
          WHERE is_active = 1";
        $dao = \CRM_Core_DAO::executeQuery($sql);
        $dao->fetch();
        $activePetitions = $dao->count;
        if ($activePetitions == 0) {
          $report = 'Disable and Delete, no active petitions';
        } 
        else {
          $report = "Sorry... ${activePetitions} active petitions, time to dive in";
        }
      }
      else {
        $report = 'Uninstall - already disabled';
      }
    }
  }
  drush_log($site  . ': ' . $report, 'error');
}

/**
 * Rebuild the multi-lingual schema 
 *
 * Check to make sure the site is multi-lingual and if so, rebuild the schema.
 * This somehow doesn't get properly run when we upgrade.
 */

function drush_pbd_rebuild_multilingual_schema() {
  _civicrm_init();
  $sql = "SELECT locales FROM civicrm_domain WHERE locales IS NOT NULL";
  $dao = CRM_Core_DAO::executeQuery($sql);
  if ($dao->N == 0) {
    return TRUE;
  }
  $dao->fetch();
  $langs = explode(CRM_Core_DAO::VALUE_SEPARATOR, $dao->locales);
  CRM_Core_I18n_Schema::rebuildMultilingualSchema($langs);
  drush_log(dt("Updated schema with @langs", array('@langs' => print_r($langs, TRUE))), 'info');
}
/**
 * Restore dedupe rules.
 */
function drush_pbd_restore_dedupe_rules() {
  _civicrm_init();
  $sql_rule = "INSERT INTO civicrm_dedupe_rule VALUES (4,2,'civicrm_contact','organization_name',NULL,10),(5,2,'civicrm_email','email',NULL,10),(6,3,'civicrm_contact','household_name',NULL,10),(7,3,'civicrm_email','email',NULL,10),(9,5,'civicrm_contact','organization_name',NULL,10),(10,5,'civicrm_email','email',NULL,10),(11,6,'civicrm_contact','household_name',NULL,10),(12,6,'civicrm_email','email',NULL,10)";
  $sql_rule_group = "INSERT INTO civicrm_dedupe_rule_group VALUES (1,'Individual',20,'Supervised','IndividualSupervised','Name and Email (reserved)',1),(2,'Organization',10,'Supervised','OrganizationSupervised','Name and Email',0),(3,'Household',10,'Supervised','HouseholdSupervised','Name and Email',0),(4,'Individual',10,'Unsupervised','IndividualUnsupervised','Email (reserved)',1),(5,'Organization',10,'Unsupervised','OrganizationUnsupervised','Name and Email',0),(6,'Household',10,'Unsupervised','HouseholdUnsupervised','Name and Email',0),(7,'Individual',10,'General','IndividualGeneral','Name and Address (reserved)',1)";

  CRM_Core_DAO::executeQuery("DELETE FROM civicrm_dedupe_rule");
  CRM_Core_DAO::executeQuery("DELETE FROM civicrm_dedupe_rule_group");
  CRM_Core_DAO::executeQuery($sql_rule_group);
  CRM_Core_DAO::executeQuery($sql_rule);
}

/**
 * Find any missing CiviCRM indices and add them.
 */
function drush_pbd_add_missing_indices() {
  _civicrm_init();

  // Ensure uf_group names are unique
  $sql = "SELECT id, name, count(*) c FROM civicrm_uf_group GROUP BY name HAVING c > 1;";
  $dao = CRM_Core_DAO::executeQuery($sql);
  while($dao->fetch()) {
    $sql = "UPDATE civicrm_uf_group SET name = %0 WHERE name = %1 AND id != %2";
    $params = array(
      0 => array($dao->name . '1', 'String'),
      1 => array($dao->name, 'String'),
      2 => array($dao->id, 'Integer')
    );
    CRM_Core_DAO::executeQuery($sql, $params);
  }
  $missing_indices = CRM_Core_BAO_SchemaHandler::getMissingIndices();
  if($missing_indices) {
    CRM_Core_BAO_SchemaHandler::createMissingIndices($missing_indices);
  }
  else {
    drush_log("No missing indices", 'info');
  }
}

/**
 * Ensure we run as admin user
 *
 * Some configuration steps check permissions and we don't want to fail
 * on those.
 *
 */
function pbd_run_as_admin() {
  // Ensure we operate with admin privs
  global $user;
  $user = user_load(1);
}

/**
 * FIXME: I think both of these commands are now
 * in civicrm.drush.inc
 **/
function drush_pbd_enable_debugging() {
  pbd_change_debugging(1);
}

function drush_pbd_disable_debugging() {
  pbd_change_debugging(0);
}

function drush_pbd_schema_version() {
  _civicrm_init();
  drush_print(CRM_Core_BAO_Domain::version());
}

function drush_pbd_code_version() {
  _civicrm_init();
  drush_print(CRM_Utils_System::version());
}

/**
 *
 * Used to obscure data so it can be sent to CiviCRM
 * core team members to fix bugs. This was taken from
 * the encryptDB.php script.
 *
 * FIXME: This should be re-posted upstream to civicrm.drush.inc.
 *
 **/
function drush_pbd_obscure() {
  $prompt = "Are you sure you want to proceed? This will destroy your database!";
  $default = 'n';
  $required = TRUE;
  $response = drush_prompt($prompt, $default, $required);
  if($response != 'y') {
    drush_print("Not running.");
    return;
  }
  drush_print("Obscuring database values...");
  define('CRM_ENCRYPT', 1);
  define('CRM_SETNULL', 2);
  $tables = array(
    'civicrm_contact' => array(
      'first_name' => CRM_ENCRYPT,
      'last_name' => CRM_ENCRYPT,
      'organization_name' => CRM_ENCRYPT,
      'household_name' => CRM_ENCRYPT,
      'sort_name' => CRM_ENCRYPT,
      'display_name' => CRM_ENCRYPT,
      'legal_name' => CRM_ENCRYPT,
    ),
    'civicrm_address' => array(
      'street_address' => CRM_ENCRYPT,
      'supplemental_address_1' => CRM_ENCRYPT,
      'supplemental_address_2' => CRM_ENCRYPT,
      'city' => CRM_ENCRYPT,
      'postal_code' => CRM_SETNULL,
      'postal_code_suffix' => CRM_SETNULL,
      'geo_code_1' => CRM_SETNULL,
      'geo_code_2' => CRM_SETNULL,
    ),
    'civicrm_website' => array(
      'url' => CRM_ENCRYPT,
    ),
    'civicrm_email' => array(
      'email' => CRM_ENCRYPT,
    ),
    'civicrm_phone' => array(
      'phone' => CRM_ENCRYPT,
    ),
  );

  foreach ($tables as $tableName => $fields) {
    $clauses = array();
    foreach ($fields as $fieldName => $action) {
      if ($action == CRM_ENCRYPT) {
        $clauses[] = "$fieldName = md5($fieldName)";
      }
      elseif ($action == CRM_SETNULL) {
        $clauses[] = "$fieldName = null";
      }
    }

    if (!empty($clauses)) {
      $clause = implode(',', $clauses);
      $query = "UPDATE $tableName SET $clause";
      CRM_Core_DAO::executeQuery($query);
    }
  }
}


function drush_pbd_update_assessments() {
  _civicrm_init();
  $key =  'net.ourpowerbase.assessment';
  $ext_system = CRM_Extension_System::singleton();
  $manager = $ext_system->getManager();
  $status = $manager->getStatus($key);
  if($status != $manager::STATUS_INSTALLED) {
    drush_log(dt("The net.ourpowerbase.assess extension is not installed."), 'error');
    return;
  }
  if(!assessment_is_configured()) {
    drush_log(dt("The net.ourpowerbase.assess extension is installed, but not configured."), 'error');
    return;
  }
  $activity_contact_types = CRM_Core_OptionGroup::values('activity_contacts', FALSE, FALSE, FALSE, NULL, 'name');
  $target_id = CRM_Utils_Array::key('Activity Targets', $activity_contact_types);

  // List of contacts with at least one activity with a non-empty engagement level.
  $contacts_sql = "SELECT DISTINCT contact_id FROM civicrm_activity a JOIN ".
    "civicrm_activity_contact ac ON a.id = ac.activity_id WHERE ".
    "a.is_deleted = 0 AND a.engagement_level IS NOT NULL AND ac.record_type_id = %1";
  $contacts_params = array(1 => array($target_id, 'Integer'));

  // Query for getting the relevant engagement level
  $engagement_sql = "SELECT a.engagement_level, a.activity_date_time FROM civicrm_activity a ".
      "JOIN civicrm_activity_contact ac ON a.id = ac.activity_id ".
      "WHERE a.is_deleted = 0 AND a.engagement_level IS NOT NULL AND ".
      "a.activity_date_time < NOW() AND ac.contact_id = %1 ".
      "AND ac.record_type_id = %2 " .
      "ORDER BY activity_date_time DESC LIMIT 1";
  $contacts_dao = CRM_Core_DAO::executeQuery($contacts_sql, $contacts_params);
  while($contacts_dao->fetch()) {
    $engagement_params = array(
      1 => array($contacts_dao->contact_id, 'Integer'),
      2 => array($target_id, 'Integer')
    );
    $engagement_dao = CRM_Core_DAO::executeQuery($engagement_sql, $engagement_params);
    $engagement_dao->fetch();
    if($engagement_dao->N != 0) {
      drush_log("Updating contact ($contacts_dao->contact_id) with: $engagement_dao->activity_date_time and $engagement_dao->engagement_level\n");
      assessment_update_assessment(
        $contacts_dao->contact_id,
        $engagement_dao->engagement_level,
        $engagement_dao->activity_date_time
      );
    }
  }
}

/**
 * For the given contact id, output all subscription links.
 *
 * These are the links that a user can follow to update their
 * own subscription (recurring contribution).
 */
function drush_pbd_subscription_links() {
  _civicrm_init();
  $id = drush_get_option('id');

  if(empty($id)) {
    drush_log(dt("Please pass the contact id via the id parameter."), 'error');
    return false;
  }
  $contributionStatus = CRM_Contribute_PseudoConstant::contributionStatus(NULL, 'name');
  $in_progress_id = array_search('In Progress', $contributionStatus);

  $params = array(
    0 => array($in_progress_id, 'Integer'),
    1 => array($id, 'Integer')
  );

  // Try to find an in-progress recurring contribution matching this id.
  $sql = "SELECT c.id AS id, first_name, last_name, r.contribution_status_id FROM civicrm_contribution c
    JOIN civicrm_contribution_recur r ON c.contribution_recur_id = r.id
    JOIN civicrm_contact co ON r.contact_id = co.id
    WHERE r.contact_id = %1
    AND is_deceased = 0 
    ORDER BY r.id DESC LIMIT 1";
  $dao = CRM_Core_DAO::executeQuery($sql, $params);
  $dao->fetch();
  if($dao->N == 0) {
    drush_log("Failed to find a matching recurring contribution record for that id.", 'error');
    return;
  }
  $contribution_id = $dao->id;
  $entity = 'contribution';
  $paymentObject = CRM_Financial_BAO_PaymentProcessor::getProcessorForEntity($contribution_id, 'contribute', 'obj');
  $update_url = $paymentObject->subscriptionURL($contribution_id, $entity, 'update');
  $cancel_url = $paymentObject->subscriptionURL($contribution_id, $entity, 'cancel');
  $billing_url = $paymentObject->subscriptionURL($contribution_id, $entity, 'billing');
  drush_print("Name: " . $dao->first_name . " " . $dao->last_name . "(" . $dao->contribution_status_id . ")");
  drush_print("Update: $update_url");
  drush_print("Cancel: $cancel_url");
  drush_print("Billing: $billing_url");

}
/**
 * Prepare a new theme.
 *
 * When creating a new theme, a number of steps have to be taken to enable
 * it to work properly as a custom theme.
 */
function drush_pbd_prepare_theme() {
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
  $settings = array(
    'toggle_logo' => 1,
    'toggle_name' => 0,
    'toggle_slogan' => 0,
    'toggle_node_user_picture' => 0,
    'toggle_comment_user_verification' => 0,
    'toggle_favicon' => 0,
    'toggle_main_menu' => 0,
    'toggle_secondary_menu' => 0,
    'default_logo' => 0,
    'logo_path' => "sites/default/themes/${name}/${name}-logo.png"
  );
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
/*
 * ***************************************************************
 *
 * Helper functions below
 *
 * ***************************************************************
 */

/**
 * Helper function for storing persistant data
 * for pbd.
 **/
function pbd_set_setting($key, $value) {
  $group = 'PowerBase Drush';
  pbd_setting($key, $value, $group);
}

/**
 * Helper function for getting persistant data
 * for this extension.
 **/
function pbd_get_setting($key, $default = NULL) {
  $params['name'] = $key;
  $ret = civicrm_api3('Setting', 'getvalue', $params);
  if(empty($ret)) return $default;
  return $ret;
}


function pbd_is_localhost($host) {
  if($host == 'localhost') return true;
  return false;
}

// designed to be used in array_walk function below,
// it strips nbsp; from the value passed in
function opb_strip_nbsp(&$value, $key) {
  $value = str_replace('&nbsp;','',$value);
}

function pbd_change_debugging($status) {
  _civicrm_init();
  require_once "CRM/Core/BAO/ConfigSetting.php";
  $params = Array ( 'debug' => $status, 'backtrace' => $status);
  CRM_Core_BAO_ConfigSetting::add($params);
}

function pbd_adjust_display_preferences() {
  pbd_fix_contact_edit_options();
  pbd_fix_user_dashboard_options();

}

function pbd_fix_contact_edit_options() {
  _civicrm_init();

  // Get the id of the option group
  $results = civicrm_api("OptionGroup","get", array ('version' => 3,'name' => 'contact_edit_options' ));

  // Recreate options so we can ensure proper weight
  if($results['is_error'] == 0) {
    $values = array_pop($results['values']);
    $id = $values['id'];

    // Now initialize new $values array to be used for the option values.
    $values = array(
      'Address' => array(
        'label' => 'Address',
        'weight' => 6,
        'filter' => 0,
      ),
      'CommunicationPreferences' => array(
        'label' => 'Communication Preferences',
        'weight' => 7,
        'filter' => 0,
      ),
      'Demographics' => array(
        'label' => 'Demographics',
        'weight' => 8,
        'filter' => 0,
      ),
      'CustomData' => array(
        'label' => 'Custom Data',
        'weight' => 9,
        'filter' => 0,
      ),
      'Notes' => array(
        'label' => 'Notes',
        'weight' => 10,
        'filter' => 0,
      ),
      'TagsAndGroups'  => array(
        'label' => 'Tags and Groups',
        'weight' => 11,
        'filter' => 0,
      ),
      'Email' => array(
        'label' => 'Email',
        'weight' => 12,
        'filter' => 1,
      ),
      'Phone' => array(
        'label' => 'Phone',
        'weight' => 12,
        'filter' => 1,
      ),
      'IM' => array(
        'label' => 'IM',
        'weight' => 13,
        'filter' => 1,
      ),
      'Website' => array(
        'label' => 'Website',
        'weight' => 14,
        'filter' => 1,
      ),
      'OpenID' => array(
        'label' => 'Open ID',
        'weight' => 14,
        'filter' => 1,
      ),
    );
    // FIXME: this should be removed after the 4.5 upgrade
    $dbVer = db_query('select version from {civicrm_domain} where id = :id', array(':id' => CIVICRM_DOMAIN_ID))->fetchField();
    if (substr($dbVer, 0, 3) >= '4.5') {
      // These are extra options added in 4.5
      $four_five_values = array(
        'Prefix' => array(
          'label' => 'Prefix',
          'weight' => 15,
          'filter' => 2,
        ),
        'Formal Title' => array(
          'label' => 'Formal Title',
          'weight' => 16,
          'filter' => 2,
        ),
        'First Name' => array(
          'label' => 'First Name',
          'weight' => 17,
          'filter' => 2,
        ),
        'Middle Name' => array(
          'label' => 'Middle Name',
          'weight' => 18,
          'filter' => 2,
        ),
        'Last Name' => array(
          'label' => 'Last Name',
          'weight' => 19,
          'filter' => 2,
        ),
        'Suffix' => array(
          'label' => 'Suffix',
          'weight' => 20,
          'filter' => 2,
        ),
      );
      $values = array_merge($values, $four_five_values);
    }

    // Resave all values.
    $params = array(
      'version' => 3,
      'option_group_id' => $id,
    );

    // Keep an array of ids - we'll need to that to save below when we indicate
    // which of these options should be displayed (the option value indicates the
    // weight, not whether or not to display them).
    $ids = array();
    // re-create each param
    while(list($k,$v) = each($values)) {
      // blow away existing item
      $sql = "DELETE FROM civicrm_option_value WHERE option_group_id = %0 AND name = %1";
      $query_params = array ( 0 => array($id, 'Integer'), 1 => array($k, 'String'));
      CRM_Core_DAO::executeQuery($sql, $query_params);

      $params['weight'] = $v['weight'];
      $params['name'] = $k;
      $params['label'] = $v['label'];
      $params['filter'] = $v['filter'];
      $create_result = civicrm_api("OptionValue","create", $params);
      $create_value = array_pop($create_result['values']);
      if($k != 'Formal Title' && $k != 'OpenID') {
        // We don't display Formal Title
        $ids[] = $create_value['value'];
      }
    }

    // In addition, we want to add other options to be checked by default
    $sql = "SELECT value FROM civicrm_option_value WHERE option_group_id = %0 AND name IN ('Email','Phone','IM', 'Website')";
    $query_params = array ( 0 => array($id, 'Integer'));
    $dao = CRM_Core_DAO::executeQuery($sql, $query_params);
    while($dao->fetch()) {
      $ids[] = $dao->value;
    }
    // now ensure the right ones are checked
    pbd_setting('contact_edit_options', $ids, 'CiviCRM Preferences');
  }
}

function pbd_fix_user_dashboard_options() {
  _civicrm_init();
  // now run for user_dashboard_options
  $results = civicrm_api("OptionGroup","get", array ('version' => 3,'name' => 'user_dashboard_options' ));
  if($results['is_error'] == 0) {
    $values = array_pop($results['values']);
    $id = $values['id'];

    $sql = "SELECT value FROM civicrm_option_value WHERE option_group_id = %0";
    $query_params = array ( 0 => array($id, 'Integer'));
    $dao = CRM_Core_DAO::executeQuery($sql, $query_params);
    $ids = array();
    while($dao->fetch()) {
      $ids[] = $dao->value;
    }
    $codeVer = CRM_Utils_System::version();
    if(version_compare($codeVer, '4.7', '<')) {
      // We need a different format for the values if we're using the pre 4.7 method.
      $ids = CRM_Core_DAO::VALUE_SEPARATOR . implode(CRM_Core_DAO::VALUE_SEPARATOR, $ids) . CRM_Core_DAO::VALUE_SEPARATOR;
    }
    pbd_setting('user_dashboard_options', $ids, 'CiviCRM Preferences');
  }
}

/**
 * Return the contact id of the user running this script.
 *
 * Some things we create have to have a contact_id for the contact
 * that created them. This function tries to return the most sensible
 * contact id for that purpose.
 */
function pbd_get_my_contact_id() {
  // First see if there's a contact that is related to the Drupal user
  // with UID 1.
  $params = array(
    'uf_id' => 1
  );
  try {
    $result = civicrm_api3('UFMatch', 'get', $params);
  }
  catch (CiviCRM_API3_Exception $e) {
    drush_log(dt("Failed to do UFMatch lookup."), 'error');
    drush_log($e->getMessage(), 'error');
    return FALSE;
  }
  if($result['count'] == 1) {
    $value = array_pop($result['values']);
    return $value['contact_id'];
  }
  // Try to pick the contact_id with the lowest number
  $sql = "SELECT MIN(id) AS min_id FROM civicrm_contact WHERE is_deleted = 0";
  $dao = CRM_Core_DAO::executeQuery($sql);
  $dao->fetch();
  if(!empty($dao->min_id)) return $dao->min_id;
  // Give up. Return 0. Hope for the best.
  drush_log(dt("Failed to find my contact id."), 'error');
  return 0;
}

// designed to be run on install to delete all default tags
function pbd_delete_all_tags() {
  _civicrm_init();
  $sql = "DELETE FROM civicrm_tag";
  CRM_Core_DAO::executeQuery( $sql );
}

// Wrapper for pm_module_list/drush_module_list.
// pm_module_list is drush 5.0. drush_module_list
// is later versions.
function pbd_module_list() {
  if(function_exists('pm_module_list')) {
    return pm_module_list();
  }
  else {
    return drush_module_list();
  }

}

function pbd_get_voter_info_custom_group_params() {
  $source = array(
    'name' => 'Voter_Info',
    'title' => 'Voter Info',
    'extends' => 'Individual',
    'style' => 'Tab',
    'collapse_display' => '0',
    'help_pre' => '',
    'help_post' => '',
    'weight' => '7',
    'is_active' => '1',
    'is_multiple' => '0',
    'collapse_adv_display' => '0'
  );
  return $source;
}

function pbd_get_voter_info_custom_field_params($custom_group_id,$option_group_ids) {
  $source[] = array(
    'custom_group_id' => $custom_group_id,
    'label' => 'Precinct',
    'data_type' => 'String',
    'html_type' => 'Text',
    'is_required' => '0',
    'is_searchable' => '1',
    'is_search_range' => '0',
    'weight' => '124',
    'is_active' => '1',
    'is_view' => '0',
    'text_length' => '255',
    'date_format' => array(
      '0' => 'M',
      '1' => 'd',
      '2' => 'Y',
    ),
    'note_columns' => '60',
    'note_rows' => '4',
  );

  $source[] = array(
    'custom_group_id' => $custom_group_id,
    'label' => 'State Senate District',
    'data_type' => 'String',
    'html_type' => 'Text',
    'is_required' => '0',
    'is_searchable' => '1',
    'is_search_range' => '0',
    'weight' => '140',
    'is_active' => '1',
    'is_view' => '0',
    'text_length' => '255',
    'note_columns' => '60',
    'note_rows' => '4',
  );
  $source[] = array(
    'custom_group_id' => $custom_group_id,
    'label' => 'City District',
    'data_type' => 'String',
    'html_type' => 'Text',
    'is_required' => '0',
    'is_searchable' => '1',
    'is_search_range' => '0',
    'weight' => '133',
    'is_active' => '1',
    'is_view' => '0',
    'text_length' => '255',
    'date_format' => array(
      '0' => 'M',
      '1' => 'd',
      '2' => 'Y',
    ),
    'note_columns' => '60',
    'note_rows' => '4',
  );
  $source[] = array(
    'custom_group_id' => $custom_group_id,
    'label' => 'School District',
    'data_type' => 'String',
    'html_type' => 'Text',
    'is_required' => '0',
    'is_searchable' => '1',
    'is_search_range' => '0',
    'weight' => '149',
    'is_active' => '1',
    'is_view' => '0',
    'text_length' => '255',
    'note_columns' => '60',
    'note_rows' => '4',
  );
  $source[] = array(
    'custom_group_id' => $custom_group_id,
    'label' => 'County District',
    'data_type' => 'String',
    'html_type' => 'Text',
    'is_required' => '0',
    'is_searchable' => '1',
    'is_search_range' => '0',
    'weight' => '154',
    'is_active' => '1',
    'is_view' => '0',
    'text_length' => '255',
    'note_columns' => '60',
    'note_rows' => '4',
  );

  $source[] = array(
    'custom_group_id' => $custom_group_id,
    'label' => 'Congressional District',
    'data_type' => 'String',
    'html_type' => 'Text',
    'is_required' => '0',
    'is_searchable' => '1',
    'is_search_range' => '0',
    'weight' => '158',
    'is_active' => '1',
    'is_view' => '0',
    'text_length' => '255',
    'date_format' => array(
      '0' => 'M',
      '1' => 'd',
      '2' => 'Y',
    ),
    'note_columns' => '60',
    'note_rows' => '4',
  );
  $source[] = array(
    'custom_group_id' => $custom_group_id,
    'label' => 'Party Registration',
    'data_type' => 'String',
    'html_type' => 'Radio',
    'is_required' => '0',
    'is_searchable' => '1',
    'is_search_range' => '0',
    'weight' => '8',
    'is_active' => '1',
    'is_view' => '0',
    'text_length' => '255',
    'date_format' => array(
      '0' => 'M',
      '1' => 'd',
      '2' => 'Y',
    ),
    'note_columns' => '60',
    'note_rows' => '4',
    'option_group_id' => $option_group_ids['party_registration'],
  );
  $source[] = array(
    'custom_group_id' => $custom_group_id,
    'label' => 'If other party...',
    'data_type' => 'String',
    'html_type' => 'Text',
    'is_required' => '0',
    'is_searchable' => '1',
    'is_search_range' => '0',
    'weight' => '30',
    'is_active' => '1',
    'is_view' => '0',
    'text_length' => '255',
    'date_format' => array(
      '0' => 'M',
      '1' => 'd',
      '2' => 'Y',
    ),
    'note_columns' => '60',
    'note_rows' => '4',
  );
  $source[] = array(
    'custom_group_id' => $custom_group_id,
    'label' => 'County Name',
    'data_type' => 'String',
    'html_type' => 'Select',
    'is_required' => '0',
    'is_searchable' => '1',
    'is_search_range' => '0',
    'weight' => '166',
    'is_active' => '1',
    'is_view' => '0',
    'text_length' => '255',
    'date_format' => array(
      '0' => 'M',
      '1' => 'd',
      '2' => 'Y',
    ),
    'note_columns' => '60',
    'note_rows' => '4',
    'option_group_id' => $option_group_ids['county_name'],
  );
  $source[] = array(
    'custom_group_id' => $custom_group_id,
    'name' => 'Van_ID',
    'label' => 'Van ID',
    'data_type' => 'String',
    'html_type' => 'Text',
    'is_required' => '0',
    'is_searchable' => '1',
    'is_search_range' => '0',
    'weight' => '188',
    'is_active' => '1',
    'is_view' => '0',
    'text_length' => '255',
    'note_columns' => '60',
    'note_rows' => '4',
  );
  $source[] = array(
    'custom_group_id' => $custom_group_id,
    'name' => 'DW_ID',
    'label' => 'DW ID',
    'data_type' => 'String',
    'html_type' => 'Text',
    'is_required' => '0',
    'is_searchable' => '1',
    'is_search_range' => '0',
    'weight' => '186',
    'is_active' => '1',
    'is_view' => '0',
    'text_length' => '255',
    'note_columns' => '60',
    'note_rows' => '4',
  );
  $source[] = array(
    'custom_group_id' => $custom_group_id,
    'name' => 'CountyFileID',
    'label' => 'CountyFileID',
    'data_type' => 'String',
    'html_type' => 'Text',
    'is_required' => '0',
    'is_searchable' => '0',
    'is_search_range' => '0',
    'weight' => '193',
    'is_active' => '1',
    'is_view' => '0',
    'text_length' => '255',
    'note_columns' => '60',
    'note_rows' => '4',
  );
  $source[] = array(
    'custom_group_id' => $custom_group_id,
    'name' => 'PrecinctName',
    'label' => 'PrecinctName',
    'data_type' => 'String',
    'html_type' => 'Text',
    'is_required' => '0',
    'is_searchable' => '1',
    'is_search_range' => '0',
    'weight' => '33',
    'is_active' => '1',
    'is_view' => '0',
    'text_length' => '255',
    'note_columns' => '60',
    'note_rows' => '4',
  );
  $source[] = array(
    'custom_group_id' => $custom_group_id,
    'name' => 'Voter_Registration_Date',
    'label' => 'Voter Registration Date',
    'data_type' => 'Date',
    'html_type' => 'Select Date',
    'is_required' => '0',
    'is_searchable' => '1',
    'is_search_range' => '1',
    'weight' => '171',
    'is_active' => '1',
    'is_view' => '0',
    'text_length' => '255',
    'date_format' => 'mm/dd/yy',
    'note_columns' => '60',
    'note_rows' => '4',
  );
  $source[] = array(
    'custom_group_id' => $custom_group_id,
    'name' => 'State_Voter_File_ID',
    'label' => 'State Voter File ID',
    'data_type' => 'String',
    'html_type' => 'Text',
    'is_required' => '0',
    'is_searchable' => '0',
    'is_search_range' => '0',
    'weight' => '192',
    'is_active' => '1',
    'is_view' => '0',
    'text_length' => '255',
    'note_columns' => '60',
    'note_rows' => '4',
  );
  $source[] = array(
    'custom_group_id' => $custom_group_id,
    'name' => 'State_Assembly_District',
    'label' => 'State Assembly District',
    'data_type' => 'String',
    'html_type' => 'Text',
    'is_required' => '0',
    'is_searchable' => '1',
    'is_search_range' => '0',
    'weight' => '135',
    'is_active' => '1',
    'is_view' => '0',
    'text_length' => '255',
    'note_columns' => '60',
    'note_rows' => '4',
  );
  $source[] = array(
    'custom_group_id' => $custom_group_id,
    'name' => 'Elections_2011',
    'label' => 'Elections 2011',
    'data_type' => 'String',
    'html_type' => 'CheckBox',
    'default_value' => array(
      '0' => 'GY',
    ),
    'is_required' => '0',
    'is_searchable' => '1',
    'is_search_range' => '0',
    'weight' => '172',
    'is_active' => '1',
    'is_view' => '0',
    'text_length' => '255',
    'note_columns' => '60',
    'note_rows' => '4',
    'option_group_id' => $option_group_ids['election'],
  );
  $source[] = array(
    'custom_group_id' => $custom_group_id,
    'name' => 'Elections_2010',
    'label' => 'Elections 2010',
    'data_type' => 'String',
    'html_type' => 'CheckBox',
    'default_value' => array(
      '0' => 'GY',
    ),
    'is_required' => '0',
    'is_searchable' => '1',
    'is_search_range' => '0',
    'weight' => '173',
    'is_active' => '1',
    'is_view' => '0',
    'text_length' => '255',
    'note_columns' => '60',
    'note_rows' => '4',
    'option_group_id' => $option_group_ids['election'],

  );
  $source[] = array(
    'custom_group_id' => $custom_group_id,
    'name' => 'Elections_2009',
    'label' => 'Elections 2009',
    'data_type' => 'String',
    'html_type' => 'CheckBox',
    'default_value' => array(
      '0' => 'GY',
    ),
    'is_required' => '0',
    'is_searchable' => '1',
    'is_search_range' => '0',
    'weight' => '174',
    'is_active' => '1',
    'is_view' => '0',
    'text_length' => '255',
    'note_columns' => '60',
    'note_rows' => '4',
    'option_group_id' => $option_group_ids['election'],

  );
  $source[] = array(
    'custom_group_id' => $custom_group_id,
    'name' => 'Elections_2008',
    'label' => 'Elections 2008',
    'data_type' => 'String',
    'html_type' => 'CheckBox',
    'default_value' => array(
      '0' => 'GY',
    ),
    'is_required' => '0',
    'is_searchable' => '1',
    'is_search_range' => '0',
    'weight' => '176',
    'is_active' => '1',
    'is_view' => '0',
    'text_length' => '255',
    'note_columns' => '60',
    'note_rows' => '4',
    'option_group_id' => $option_group_ids['election'],
  );
  $source[] = array(
    'custom_group_id' => $custom_group_id,
    'name' => 'Elections_2007',
    'label' => 'Elections 2007',
    'data_type' => 'String',
    'html_type' => 'CheckBox',
    'default_value' => array(
      '0' => 'GY',
    ),
    'is_required' => '0',
    'is_searchable' => '1',
    'is_search_range' => '0',
    'weight' => '178',
    'is_active' => '1',
    'is_view' => '0',
    'text_length' => '255',
    'note_columns' => '60',
    'note_rows' => '4',
    'option_group_id' => $option_group_ids['election'],
  );
  $source[] = array(
    'custom_group_id' => $custom_group_id,
    'name' => 'Elections_2006',
    'label' => 'Elections 2006',
    'data_type' => 'String',
    'html_type' => 'CheckBox',
    'default_value' => array(
      '0' => 'GY',
    ),
    'is_required' => '0',
    'is_searchable' => '1',
    'is_search_range' => '0',
    'weight' => '180',
    'is_active' => '1',
    'is_view' => '0',
    'text_length' => '255',
    'note_columns' => '60',
    'note_rows' => '4',
    'option_group_id' => $option_group_ids['election'],
  );
  $source[] = array(
    'custom_group_id' => $custom_group_id,
    'name' => 'Elections_2005',
    'label' => 'Elections 2005',
    'data_type' => 'String',
    'html_type' => 'CheckBox',
    'default_value' => array(
      '0' => 'GY',
    ),
    'is_required' => '0',
    'is_searchable' => '1',
    'is_search_range' => '0',
    'weight' => '182',
    'is_active' => '1',
    'is_view' => '0',
    'text_length' => '255',
    'note_columns' => '60',
    'note_rows' => '4',
    'option_group_id' => $option_group_ids['election'],
  );
  $source[] = array(
    'custom_group_id' => $custom_group_id,
    'name' => 'Date_of_Voter_Data',
    'label' => 'Date of Voter Data',
    'data_type' => 'Date',
    'html_type' => 'Select Date',
    'is_required' => '0',
    'is_searchable' => '1',
    'is_search_range' => '1',
    'weight' => '183',
    'is_active' => '1',
    'is_view' => '0',
    'text_length' => '255',
    'date_format' => 'mm/dd/yy',
    'note_columns' => '60',
    'note_rows' => '4',
  );
  return $source;
}

function pbd_get_election_option_group_params() {
  $source = array(
    'name' => 'election',
    'title' => 'Election',
    'is_reserved' => '1',
    'is_active' => '1'
  );
  return $source;
}

function pbd_get_voter_history_option_group_params() {
  $source = array(
    'name' => 'voter_history_values',
    'title' => 'Voter History Values',
    'is_reserved' => '1',
    'is_active' => '1'
  );
  return $source;
}

function pbd_get_county_name_option_group_params() {
  $source = array(
    'name' => 'county_name_options',
    'title' => 'County Name Options',
    'is_reserved' => '1',
    'is_active' => '1'
  );
  return $source;
}

function pbd_get_party_registration_option_group_params() {
  $source = array(
    'name' => 'party_registration_options',
    'title' => 'Party Registration Options',
    'is_reserved' => '1',
    'is_active' => '1'
  );
  return $source;
}

function pbd_get_party_registration_option_value_params($option_group_id) {
  $source[] = array(
    'option_group_id' => $option_group_id,
    'label' => 'Democrat',
    'value' => 'D',
    'name' => 'Democrat',
    'is_default' => '0',
    'weight' => '1',
    'is_optgroup' => '0',
    'is_reserved' => '0',
    'is_active' => '1'
  );
  $source[] = array(
    'option_group_id' => $option_group_id,
    'label' => 'Green',
    'value' => 'G',
    'name' => 'Green',
    'is_default' => '0',
    'weight' => '3',
    'is_optgroup' => '0',
    'is_reserved' => '0',
    'is_active' => '1'
  );
  $source[] = array(
    'option_group_id' => $option_group_id,
    'label' => 'Independent',
    'value' => 'I',
    'name' => 'Independent',
    'is_default' => '0',
    'weight' => '4',
    'is_optgroup' => '0',
    'is_reserved' => '0',
    'is_active' => '1'
  );
  $source[] = array(
    'option_group_id' => $option_group_id,
    'label' => 'Libertarian',
    'value' => 'L',
    'name' => 'Libertarian',
    'is_default' => '0',
    'weight' => '5',
    'is_optgroup' => '0',
    'is_reserved' => '0',
    'is_active' => '1'
  );
  $source[] = array(
    'option_group_id' => $option_group_id,
    'label' => 'Other',
    'value' => 'O',
    'name' => 'Other',
    'is_default' => '0',
    'weight' => '7',
    'is_optgroup' => '0',
    'is_reserved' => '0',
    'is_active' => '1'
  );
  $source[] = array(
    'option_group_id' => $option_group_id,
    'label' => 'Republican',
    'value' => 'R',
    'name' => 'Republican',
    'is_default' => '0',
    'weight' => '2',
    'is_optgroup' => '0',
    'is_reserved' => '0',
    'is_active' => '1'
  );
  $source[] = array(
    'option_group_id' => $option_group_id,
    'label' => 'Undeclared',
    'value' => 'U',
    'name' => 'Undeclared',
    'is_default' => '0',
    'weight' => '6',
    'is_optgroup' => '0',
    'is_reserved' => '0',
    'is_active' => '1'
  );
  return $source;
}

function pbd_get_election_option_value_params($option_group_id) {
  $source[] = array(
    'option_group_id' => $option_group_id,
    'label' => 'G - N',
    'value' => 'GN',
    'name' => 'G__N',
    'is_default' => '0',
    'weight' => '2',
    'is_optgroup' => '0',
    'is_reserved' => '0',
    'is_active' => '1'
  );
  $source[] = array(
    'option_group_id' => $option_group_id,
    'label' => 'G - Y',
    'value' => 'GY',
    'name' => 'G__Y',
    'is_default' => '0',
    'weight' => '1',
    'is_optgroup' => '0',
    'is_reserved' => '0',
    'is_active' => '1'
  );
  $source[] = array(
    'option_group_id' => $option_group_id,
    'label' => 'P - N',
    'value' => 'PN',
    'name' => 'P__N',
    'is_default' => '0',
    'weight' => '4',
    'is_optgroup' => '0',
    'is_reserved' => '0',
    'is_active' => '1'
  );
  $source[] = array(
    'option_group_id' => $option_group_id,
    'label' => 'P - Y',
    'value' => 'PY',
    'name' => 'P__Y',
    'is_default' => '0',
    'weight' => '3',
    'is_optgroup' => '0',
    'is_reserved' => '0',
    'is_active' => '1'
  );
  $source[] = array(
    'option_group_id' => $option_group_id,
    'label' => 'PP - N',
    'value' => 'PPN',
    'name' => 'PP__N',
    'is_default' => '0',
    'weight' => '6',
    'is_optgroup' => '0',
    'is_reserved' => '0',
    'is_active' => '1'
  );
  $source[] = array(
    'option_group_id' => $option_group_id,
    'label' => 'PP - Y',
    'value' => 'PPY',
    'name' => 'PP__Y',
    'is_default' => '0',
    'weight' => '5',
    'is_optgroup' => '0',
    'is_reserved' => '0',
    'is_active' => '1'
  );
  $source[] = array(
    'option_group_id' => $option_group_id,
    'label' => 'S - N',
    'value' => 'SN',
    'name' => 'S__N',
    'is_default' => '0',
    'weight' => '8',
    'is_optgroup' => '0',
    'is_reserved' => '0',
    'is_active' => '1'
  );
  $source[] = array(
    'option_group_id' => $option_group_id,
    'label' => 'S - Y',
    'value' => 'SY',
    'name' => 'S__Y',
    'is_default' => '0',
    'weight' => '7',
    'is_optgroup' => '0',
    'is_reserved' => '0',
    'is_active' => '1'
  );
  return $source;
}

function pbd_setup_option_group($group_params) {

  // ensure the OptionGroup exists, if not creaate it
  $option_group = pbd_return_entity('OptionGroup', $group_params);
  $option_group_id = $option_group['id'];
  return $option_group_id;
}

function pbd_setup_option_values($group_id, $value_params) {
  // check if any OptionValues exist for this option group
  $values = pbd_get_option_values($group_id);

  // only create if none exist - this allows each group to customize them if they want
  if(count($values) == 0) {
    // create default options
    reset($value_params);
    while(list($k,$v) = each($value_params)) {
      pbd_return_entity('OptionValue', $v);
    }
  }
}



function pbd_get_custom_field_set($name) {
  _civicrm_init();
  require_once ('api/api.php');
  $params = array(
    'version' => 3,
    'name' => $name,
  );

  $results = civicrm_api('CustomGroup', 'get', $params);
  if(count($results['values']) == 0) return array();
  $voter_info_custom_group = array_pop($results['values']);
  $custom_group_id = $voter_info_custom_group['id'];

  $params = array(
    'version' => 3,
    'custom_group_id' => $custom_group_id,
  );
  $results = civicrm_api('CustomField', 'get', $params);
  return $results['values'];
}

function pbd_get_option_values($id) {
  _civicrm_init();
  require_once ('api/api.php');
  $params = array(
    'version' => 3,
    'option_group_id' => $id,
  );
  $results = civicrm_api('OptionValue', 'get', $params);
  return $results['values'];
}

/**
 * Either fetch the matching entity or create it and
 * then return it.
 */
function pbd_return_entity($entity, $params, $key = 'name') {
  _civicrm_init();
  require_once ('api/api.php');

  $search_params[$key] = $params[$key];
  $params['version'] = $search_params['version'] = 3;
  $results = civicrm_api($entity, 'get', $search_params);
  if(count($results['values']) == 0) {
    $results = civicrm_api($entity, 'create', $params);
  }
  if($results['is_error'] != 0) {
    drush_print(dt("Error! ") . $results['error_message'] );
    return array();
  }
  return array_pop($results['values']);
}

/**
 * Setup default themekey rules.
 */


/**
 * Enable all third party modules that we use.
 */


/**
 * See: https://app.liquidplanner.com/space/48716/projects/show/7843451P
 *
 * On creation of a new site, ensure there is no default header to avoid
 * people accidentally sending embarrassing messages with a default
 * header set.
 *
 **/
function pbd_unset_default_header() {
  // Make the pb email footer the default
  _civicrm_init();
  $sql = "UPDATE civicrm_mailing_component SET is_default = 0 WHERE component_type = 'Header'";
  CRM_Core_DAO::executeQuery($sql);
}

function pbd_ensure_pb_email_footer_exists($settings) {
  require_once 'CRM/Mailing/BAO/Component.php';
  $search_params = array(
    'name' => $settings['name'],
  );
  $component = CRM_Mailing_BAO_Component::retrieve($search_params, $defaults);
  if(empty($component)) {
    // We need to create it
    $ids = array();
    CRM_Mailing_BAO_Component::add($settings, $ids);
    $component = CRM_Mailing_BAO_Component::retrieve($search_params, $defaults);
  }
  return $component->id;
}

function pbd_add_include_exclude_search_menu() {
  _civicrm_init();
  $label = 'Include/Exclude Search';

  require_once 'CRM/Core/BAO/Navigation.php';
  $menus = CRM_Core_BAO_Navigation::getNavigationList();
  $exists_array = preg_grep("#$label#", $menus);
  if(count($exists_array)) {
    drush_log(dt("Include/Exclude Menu already added."), 'notice');
    return;
  }
  // Get the value for the custom search - we need it for the URL
  $params = array('label' => 'CRM_Contact_Form_Search_Custom_Group');
  try {
    $value = civicrm_api3('OptionValue', 'getsingle', $params);
  }
  catch(CiviCRM_API3_Exception $e) {
    drush_log(dt("Failed to find Inclue/Exclude value."), 'error');
    drush_log($e->getMessage(), 'error');
    return;
  }
  $url = 'civicrm/contact/search/custom?csid=' . $value['value'] . '&reset=1';
  $menu_array = preg_grep('/^Search/', $menus);
  $name = array_pop($menu_array);
  $keys = array_keys($menus, $name);
  if (count($keys) != 1) {
    drush_print(dt("Failed to add Include/Exclude search menu item."), 'error');
    return;
  }
  $id = array_pop($keys);
  $params = array(
    'label' => $label,
    'url' => $url,
    'permission' => array(
      0 => 'access CiviCRM',
    ),
    'permission_operator' => 'AND',
    'is_active' => 1,
    'parent_id' => $id,
  );
  CRM_Core_BAO_Navigation::add($params);
}

function pbd_add_image_upload_menu() {
  _civicrm_init();
  $label = 'Uploaded Images';
  $url = 'sites/all/modules/civicrm/packages/kcfinder/browse.php?cms=civicrm&type=images';
  require_once 'CRM/Core/BAO/Navigation.php';
  $menus = CRM_Core_BAO_Navigation::getNavigationList();
  $exists_array = preg_grep("/$label/", $menus);
  if(count($exists_array)) {
    return;
  }
  $menu_array = preg_grep('/Customize Data and Screens/', $menus);
  $name = array_pop($menu_array);
  $keys = array_keys($menus, $name);
  if (count($keys) != 1) {
    drush_print(dt("Failed to add image upload menu item."));
    return;
  }
  $id = array_pop($keys);
  $params = array(
    'label' => $label,
    'url' => $url,
    'permission' => array(
      0 => 'access CiviCRM',
    ),
    'permission_operator' => 'AND',
    'is_active' => 1,
    'parent_id' => $id,
  );
  CRM_Core_BAO_Navigation::add($params);
}

/**
 * Ensure exported dates don't have ordinals because it confuses
 * MS Excel
 **/
function pbd_set_default_dateformat_full() {
  _civicrm_init();
  pbd_setting('dateformatFull', '%B %E, %Y');
}

/**
 * Ensure admin theme is set to default so all sites have consistent
 * look and feel in Drupal back-end and so all blocks consistently appear
 **/
function pbd_set_admin_theme() {
  variable_set('admin_theme', '0');
  variable_set('node_admin_theme', 0);
}

/**
 * We don't want webforms showing up on front page
 **/



/**
 * Ensure proper jquery version is selected.
 */
function pbd_configure_jquery_update() {
  if(module_exists('jquery_update')) {
    variable_set('jquery_update_compression_type', 'min');
    variable_set('jquery_update_jquery_admin_version', '');
    variable_set('jquery_update_jquery_cdn', 'none');
    variable_set('jquery_update_jquery_version', '1.10');
  }
}

/**
 * Confgure the autologout module default time.
 *
 * This is run once on install and can be changed via the UI. 
 **/
function pbd_configure_autologout() {
}

/**
 * Helper function. Return custom field id for the first part
 * of the custom field name.
 **/
function pbd_id_for_custom_field_name($name) {
  _civicrm_init();
  // Get the id of the ward field, which may be different
  // on each site.
  $sql = "SELECT id FROM civicrm_custom_field WHERE column_name LIKE %1";
  $params = array(
    1 => array($name . '%', 'String')
  );
  $dao = CRM_Core_DAO::executeQuery($sql, $params);
  $dao->fetch();
  if(empty($dao->id)) {
    return FALSE;
  }
  return $dao->id;
}

/**
 * Used to safely delete un-used custom fields.
 **/
function pbd_remove_custom_field_if_unused($id) {
  if(pbd_custom_field_is_unused($id)) {
    $params = array('id' => $id, 'version' => 3);
    $results = civicrm_api('CustomField','delete',$params);
    if($results['is_error'] == 0) {
      return TRUE;
    }
    else {
      drush_print(dt($results['error_message']));
    }
  }
  return FALSE;
}

/**
 * Returns FALSE (and emits error via drush_print)
 * if the custom field is used in a profile or contains
 * data. Otherwise, returns TRUE.
 **/
function pbd_custom_field_is_unused($id) {

  if(!pbd_column_is_empty($id)) {
    drush_print("Found values in the custom table.");
    return FALSE;
  }
  if(!pbd_custom_field_not_used_in_profile($id)) {
    drush_print("Field is used in a profile.");
    return FALSE;
  }
  return TRUE;
}

/**
 * Helper function - returns column name
 * and table name for custom field i.
 **/
function pbd_table_and_column_name_for_custom_field_id($id) {
  $sql = "SELECT column_name, table_name FROM civicrm_custom_field cf ".
   "JOIN civicrm_custom_group cg ON cf.custom_group_id = cg.id  WHERE cf.id = %1";
  $params = array(
    1 => array($id, 'Integer')
  );
  $dao = CRM_Core_DAO::executeQuery($sql, $params);
  $dao->fetch();
  if(empty($dao->column_name)) {
    return FALSE;
  }
  return array($dao->column_name => $dao->table_name);
}

/**
 * Helper function. Returns TRUE if the field passed
 * contains no data, so is safe to delete.
 **/
function pbd_column_is_empty($id) {
  $field_details = pbd_table_and_column_name_for_custom_field_id($id);
  if(FALSE === $field_details) {
    drush_print("Can't find custom field with that id.");
    return FALSE;
  }
  list($column_name, $table_name) = each($field_details);
  // Ensure there is no data
  $sql = "SELECT COUNT(*) AS count FROM `$table_name` WHERE ".
    "`$column_name` IS NOT NULL AND `$column_name` != ''";
  $dao = CRM_Core_DAO::executeQuery($sql);
  $dao->fetch();
  if($dao->count != 0) {
    return FALSE;
  }
  return TRUE;
}

/**
 * Helper function. Returns TRUE if the field is not being
 * used in a profile, or FALSE if it is.
 **/
function pbd_custom_field_not_used_in_profile($id) {
  $sql = "SELECT COUNT(*) AS count FROM civicrm_uf_field WHERE field_name = %1";
  $params = array(
    1 => array('custom_' . $id, 'String')
  );
  $dao = CRM_Core_DAO::executeQuery($sql, $params);
  $dao->fetch();
  if($dao->count == 0) return TRUE;
  return FALSE;
}

/**
 * Configure default civicrm cicero mappings
 **/
function pbd_configure_default_cicero_mappings() {
  // LOCAL => city_district, NATIONAL_LOWER => congressional_district,
  // STATE_LOWER => state_assembly_district, STATE_UPPER => state_senate_district
  $local = pbd_id_for_custom_field_name('city_district');
  $national_lower = pbd_id_for_custom_field_name('federal_district');
  $state_lower = pbd_id_for_custom_field_name('state_assembly_district');
  $state_upper = pbd_id_for_custom_field_name('state_senate_district');

  $map = array();
  if($local) $map['LOCAL'] = 'custom_' . $local;
  if($national_lower) $map['NATIONAL_LOWER'] = 'custom_' . $national_lower;
  if($state_lower) $map['STATE_LOWER'] = 'custom_' . $state_lower;
  if($state_upper) $map['STATE_UPPER'] = 'custom_' . $state_upper;
  variable_set('civicrm_cicero_contact_field_map', $map);
}


/**
 * Civicrm 4.3 upgrade adds civicrm news, which pulls in blog posts
 * from the CiviCRM web site (using http) and then displays the posts
 * on your dashboard. This is a security problem. This function
 * removes it from your dashboard (but it still is available via
 * /civicrm/dashboard/blog). In addition, 4.6.something added a
 * Getting Started one.
 *
 **/
function pbd_ensure_civicrm_news_removed_from_dashboard() {
  _civicrm_init();

  // Turn off CiviCRM community messages by setting the URL to empty. Note:
  // if we set it to '' - then CiviCRM resets it to the default.
  pbd_setting('communityMessagesUrl', ' ');

  // Now remove the news from the Dashboard.
  $sql = "SELECT id FROM civicrm_dashboard WHERE url LIKE 'civicrm/dashlet/blog%' OR url LIKE 'civicrm/dashlet/getting-started%'";
  $dao = CRM_Core_DAO::executeQuery($sql);
  // It's already been deleted, so return
  if($dao->N == 0) {
    return;
  }
  while($dao->fetch()) {
    // first delete all instances of this dashboard item
    $sql = "DELETE FROM civicrm_dashboard_contact WHERE dashboard_id = %0";
    $params = array(0 => array($dao->id, 'Integer'));
    CRM_Core_DAO::executeQuery($sql, $params);

    // Now delete the dashboard item so nobody will try to add it in the future
    $sql = "DELETE FROM civicrm_dashboard WHERE id = %0";
    $params = array(0 => array($dao->id, 'Integer'));
    CRM_Core_DAO::executeQuery($sql, $params);
  }
}

/**
 * Configure google recaptcha
 *
 * If values are present in the environment, add them to CiviCRM.
 * Sites should evaluate whether they want to invite Google into
 * their CRM by enabling it for a profile.
 **/
function pbd_configure_google_recaptcha() {
  $secret = getenv('LONG_GOOGLE_RECAPTCHA_SECRET_KEY');
  $site = getenv('LONG_GOOGLE_RECAPTCHA_SITE_KEY');
  if($secret) {
    drush_log(dt("Configuring Google Captcha"), 'notice');
    pbd_setting('recaptchaPrivateKey', $secret);
  }
  if($site) {
    pbd_setting('recaptchaPublicKey', $site);
  }
}

/**
 * Configure google geo/map api
 *
 */
function pbd_configure_google_geo_map() {
  $key = getenv('LONG_GOOGLE_MAP_GEO_API_KEY');
  $live = getenv('LONG_LIVE');
  drush_log(dt("Configuring Google Geo/Mapping key"), 'notice');
  pbd_setting('geoAPIKey', $key);
  pbd_setting('mapAPIKey', $key);

}

/**
 *
 * Add settings
 *
 */
function pbd_setting($key, $value, $group = NULL) {
  $params = array($key => $value);
  civicrm_api3('setting', 'create', $params);
}

/**
 * On a per group basis, set deterministic refresh mode.
 *
 * Smart Group Refresh mode is set to opportunistic by default. That means
 * the smart group cache is flushed whenever a contact changes to ensure
 * it is always up to date. That can cause some locking though. So we 
 * can optionally set it to deterministic which means it is flushed
 * once per cron job instead. If you add a group to this list you
 * *must* also:
 *  * create the cron job via the web UI (Job.group_cache_flush). 
 *  * In Search Preferences, set cache time to 15 minutes
 * Otherwise, it will never get flushed.
 */
function pbd_set_smart_group_refresh_mode() {
  $sites = array('texasenvironment', 'isaiah', 'workingwa', 'southernvision');
  if(in_array(ptp_get_subdomain(), $sites)) {
    $params = array('smart_group_cache_refresh_mode' => 'deterministic');
    drush_log("Setting smart group cache to deterministic.", 'info');
    civicrm_api3('Setting', 'create', $params);
  }
}

/**
 * Ensure we limit list of state provinces to the relevant countries.
 */
function pbd_set_state_province_limit() {
  $key = 'provinceLimit';
  $results = civicrm_api3('setting', 'get', array());
  if(empty($results['values'][$key])) {
    // Set to the US
    $params[$key] = 1228;
    civicrm_api3('setting', 'create', $params);
  }
}

/**
 * Ensure extra fonts are available for certain multi-lingual sites.
 */
function pbd_configure_additional_fonts() {
  $fonts = array(
    "batang" => "Batang",
    "dotum" => "Dotum",
    "hline" => "Hline",
    "gulim" => "Gulim",
  );
  pbd_setting('additional_fonts', $fonts, "CiviCRM Preferences");
}

/**
 * Disable external extension repos
 *
 * We don't want users to be able to install any extension,
 * just the ones we provide.
 **/
function pbd_disable_external_extension_repository() {
  pbd_setting('ext_repo_url', FALSE, "Extension Preferences");
}

/**
 * Enable remote_profile_submissions by default.
 *
 * Starting in 4.7 this is disabled. Most groups want a signup
 * form on their home page.
 */
function pbd_enable_remote_profile_submissions() {
  pbd_setting('remote_profile_submissions', TRUE);
}


/**
 * Set our own dedupe rules.
 */
function pbd_dedupe_rules() {
  // This may need to be redone. See: https://lab.civicrm.org/dev/core/issues/387
  
  // We want the default individual unsupervised rule to be first name, last name 
  // and email (e.g. what the default supervised rule is). We want the default
  // unsupervised rule (email only) to be a general rule. 
  $sql = "UPDATE civicrm_dedupe_rule_group SET used = 'General' WHERE name = 'IndividualUnsupervised'";
  CRM_Core_DAO::executeQuery($sql);
  $sql = "UPDATE civicrm_dedupe_rule_group SET used = 'Unsupervised' WHERE name = 'IndividualSupervised'";
  CRM_Core_DAO::executeQuery($sql);

  // We also want the individual supervised rule to be just first and last name.
  // Create a new rule group if it doesn't already exist.

  $sql = "SELECT id FROM civicrm_dedupe_rule_group WHERE name = 'FirstAndLastName'";
  $dao = CRM_Core_DAO::executeQuery($sql);
  if ($dao->N == 0) {
    $sql = "INSERT INTO civicrm_dedupe_rule_group SET 
      contact_type = 'Individual',
      threshold = '2',
      used = 'Supervised', 
      name = 'FirstAndLastName',
      title = 'First and Last Name'";
    CRM_Core_DAO::executeQuery($sql);

    $sql = "SELECT id FROM civicrm_dedupe_rule_group WHERE name = 'FirstAndLastName'";
    $dao = CRM_Core_DAO::executeQuery($sql);
    $dao->fetch();
    
    $sql = "INSERT INTO civicrm_dedupe_rule SET
      dedupe_rule_group_id = %0,
      rule_table = 'civicrm_contact',
      rule_field = 'first_name',
      rule_length = NULL,
      rule_weight = 1";
    $params = array(0 => array($dao->id, 'Integer'));
    CRM_Core_DAO::executeQuery($sql, $params); 
    $sql = "INSERT INTO civicrm_dedupe_rule SET
      dedupe_rule_group_id = %0,
      rule_table = 'civicrm_contact',
      rule_field = 'last_name',
      rule_length = NULL,
      rule_weight = 1";
    CRM_Core_DAO::executeQuery($sql, $params); 

  }
  // And lastly, we want to fix the organization rules.
  // By default, both supervised and unsupervised are both called "Name and Email"
  // and both are actually Name OR Email. We want unsupervised to be
  // both name and email and supervised to be just name.
  $sql = "UPDATE civicrm_dedupe_rule_group SET threshold = '20' WHERE name = 'OrganizationUnsupervised'";
  CRM_Core_DAO::executeQuery($sql);

  $sql = "SELECT id FROM civicrm_dedupe_rule_group WHERE name = 'OrganizationSupervised'";
  $dao = CRM_Core_DAO::executeQuery($sql);
  $dao->fetch();

  $sql = "DELETE FROM civicrm_dedupe_rule WHERE dedupe_rule_group_id = %0 AND rule_field = 'email'";
  CRM_Core_DAO::executeQuery($sql, array(0 => array($dao->id, 'Integer')));

  $sql = "UPDATE civicrm_dedupe_rule_group SET title = 'Name' WHERE id = %0";
  CRM_Core_DAO::executeQuery($sql, array(0 => array($dao->id, 'Integer')));

}

/**
 * quick fix to make warning about transaction legacy go away.
 */
function pbd_transactlegacy_go_away() {
  // First check if stripe is installed.
  _civicrm_init();
  $sql = "SELECT COUNT(*) AS count FROM civicrm_extension WHERE name = 'stripe' and is_active = 1";
  $dao = CRM_Core_DAO::executeQuery($sql);
  $dao->fetch();
  if ($dao->count == 1) {
    drush_log("Stripe active", 'info');
    // Insert new notice configuration if it doesn't already exist. 
    $sql = "SELECT COUNT(*) AS count FROM civicrm_status_pref WHERE name = 'checkExtensionContributiontransactlegacymjwshared_recommended'";
    $dao_pref = CRM_Core_DAO::executeQuery($sql);
    $dao_pref->fetch();
    if ($dao_pref->count == 0) {
      drush_log("Notice does not exist.", 'info');
      $sql = "INSERT INTO civicrm_status_pref VALUES( NULL, 1, 'checkExtensionContributiontransactlegacymjwshared_recommended', NULL, 3, NULL, NULL, 1)";
      CRM_Core_DAO::executeQuery($sql);
    }
  }
}

/**
 * find stripe intalls without anon ajax api permission. 
 */
function pbd_report_stripe_installs_without_anon_ajax_perm() {
  // First check if stripe is installed.
  _civicrm_init();
  $sql = "SELECT COUNT(*) AS count FROM civicrm_extension WHERE name = 'stripe' and is_active = 1";
  $dao = CRM_Core_DAO::executeQuery($sql);
  $dao->fetch();
  if ($dao->count == 1) {
    // Stripe is active
    $perms = user_role_permissions([1 => "anonymous"]);
    if (!array_key_exists('access AJAX API', $perms[1])) {
      drush_log("Access API not set: " . CIVICRM_UF_BASEURL);
    }
  }
}

/**
 * quick fix to update stripe webhooks.
 */
function pbd_stripe_webhooks() {
  // First check if stripe is installed.
  _civicrm_init();
  $sql = "SELECT COUNT(*) AS count FROM civicrm_extension WHERE name = 'stripe' and is_active = 1";
  $dao = CRM_Core_DAO::executeQuery($sql);
  $dao->fetch();
  if ($dao->count == 1) {
    drush_log("Stripe active", 'info');
    $messages = [];
    CRM_Stripe_Webhook::check($messages, TRUE);
  }
}

/**
 * clear stripe 6.6 upgrade message.
 */
function pbd_stripe_66() {
  // First check if stripe is installed.
  _civicrm_init();
  $sql = "SELECT COUNT(*) AS count FROM civicrm_extension WHERE name = 'stripe' and is_active = 1";
  $dao = CRM_Core_DAO::executeQuery($sql);
  $dao->fetch();
  if ($dao->count == 1) {
    civicrm_api3('Setting', 'create', [ 'stripe_upgrade66message' => 0 ]);
  }
}
function drush_pbd_cicero_users() {
  // Check if civicrm_cicero_sync_runs table exists.
  _civicrm_init();
  $sql = "SELECT COUNT(*) AS count  FROM information_schema.tables WHERE table_name = 'civicrm_cicero_sync_runs'";
  $dao = CRM_Core_DAO::executeQuery($sql);
  $dao->fetch();
  $latest_run = NULL;
  $last_petition = NULL;
  if ($dao->count != 0) {
    $key = variable_get('civicrm_cicero_api_key');
    $user = variable_get('civicrm_cicero_account_name');

    if (!empty($key) || !empty($user)) {
      $sql = "SELECT FROM_UNIXTIME(date_to_run) AS last_run FROM civicrm_cicero_sync_runs ORDER BY date_to_run DESC LIMIT 1";
      $dao = CRM_Core_DAO::executeQuery($sql);
      $dao->fetch();
      $last_run = substr($dao->last_run, 0, 10);
    }
  }
  // Check if civicrm_petition_email table exists.
  _civicrm_init();
  $sql = "SELECT COUNT(*) AS count  FROM information_schema.tables WHERE table_name = 'civicrm_petition_email'";
  $dao = CRM_Core_DAO::executeQuery($sql);
  $dao->fetch();
  if ($dao->count != 0) {
    $sql = "SELECT MAX(created_date) AS created FROM civicrm_survey s JOIN civicrm_petition_email e ON s.id = e.petition_id WHERE is_active = 1";
    $dao = CRM_Core_DAO::executeQuery($sql);
    $dao->fetch();
    $last_petition = substr($dao->created, 0, 10);
  }

  if (empty($last_petition) && empty($last_run)) {
    return;
  }
  drush_log(CIVICRM_UF_BASEURL . ": Last run: $last_run, Last petition: $last_petition", 'error');

}

function drush_pbd_undocufund_report() {

  $csv = drush_get_option('csv', 0);

  $greatest_need_options = CRM_Core_OptionGroup::values("use_of_funds_greatest_need_20210303195412");
  $primary_source_of_income_options = CRM_Core_OptionGroup::values("employment_industry_type_of_work_20200615112452");
  $race_options = CRM_Core_OptionGroup::values("race_and_ethnicity_cirf_defined__20210303201040");

  $missing_gender = 0;
  $missing_race = 0;
  $missing_greatest_need = 0;
  $missing_primary_source_of_income = 0;
  $missing_under_18 = 0;
  $missing_over_18 = 0;

  $result = [];
  // Get the plain contact records first.
  $sql_group = "
    SELECT DISTINCT
      c.id,
      c.first_name, 
      c.last_name
    FROM civicrm_contact c 
      JOIN civicrm_group_contact gc ON c.id = gc.contact_id
      JOIN civicrm_activity_contact ac_internal ON c.id = ac_internal.contact_id AND ac_internal.record_type_id = 3  
      JOIN civicrm_value_undocufund_on_60 internal ON ac_internal.activity_id = internal.entity_id 
      LEFT JOIN civicrm_activity_contact ac_cirf ON c.id = ac_cirf.contact_id AND ac_cirf.record_type_id = 3
      LEFT JOIN civicrm_activity a_cirf ON a_cirf.id = ac_cirf.activity_id AND a_cirf.activity_type_id = 100  
    WHERE 
      internal.disbursement_decision_501 = 1 AND 
      (
        ( gc.group_id = 654 AND a_cirf.id IS NOT NULL ) OR 
        gc.group_id = 676 OR 
        gc.group_id=687 OR 
        gc.group_id=702 OR 
        gc.group_id=705
      ) 
    ";

  $sql_decision = "
    SELECT DISTINCT
      c.id,
      c.first_name, 
      c.last_name,
      a.activity_date_time as decision_date
    FROM civicrm_contact c 
      JOIN civicrm_activity_contact ac_internal ON c.id = ac_internal.contact_id AND ac_internal.record_type_id = 3  
      JOIN civicrm_activity a ON ac_internal.activity_id = a.id
      JOIN civicrm_value_undocufund_on_60 internal ON ac_internal.activity_id = internal.entity_id 
    WHERE 
      internal.disbursement_decision_501 = 1 AND 
      a.activity_date_time > '2021-02-15' 
    ";


    $contact_dao = \CRM_Core_DAO::executeQuery($sql_decision);
    while ($contact_dao->fetch()) {
      $contact_id = $contact_dao->id;
      $record = [
        'id' => $contact_id,
        'first_name' => $contact_dao->first_name,
        'last_name' => $contact_dao->last_name,
        'decision_date' => $contact_dao->decision_date
      ];

      // These are the only params we need for each query.
      $params = [ 0 => [ $contact_id, 'Integer' ] ];

      // For each record, do a best effort to get the demographic and survey data.

      // GENDER
      $sql = "SELECT DISTINCT ov.label as gender
        FROM civicrm_value_undocufund_in_57 demo
          JOIN civicrm_option_value ov ON 
            ov.value = demo.gender_cirf_defined__525 AND ov.option_group_id = 283 
        WHERE demo.entity_id = %0";

      $dao = CRM_Core_DAO::executeQuery($sql, $params);
      $dao->fetch();
      if (!isset($dao->gender)) {
        $missing_gender++;
        $record['gender'] = '';
      }
      else {
        $record['gender'] = $dao->gender;
      }

      // RACE 
      $sql = "SELECT DISTINCT race_and_ethnicity_cirf_defined__587 as race 
        FROM civicrm_value_undocufund_in_57 demo
        WHERE demo.entity_id = %0";

      $dao = \CRM_Core_DAO::executeQuery($sql, $params);
      $dao->fetch();
      if (!isset($dao->race)) {
        $missing_race++;
        $record['race'] = '';
      }
      else {
        $race_ids = explode(CRM_Core_DAO::VALUE_SEPARATOR,$dao->race);
        $races = [];
        foreach ($race_ids as $race_id) {
          $raw_value = trim($race_id);
          if (empty($raw_value)) {
            continue;
          }
          $races[] = $race_options[$raw_value];
        }
        $record['race'] = implode(",",$races);
      }

      // NUMBER 18 AND OVER
      $sql = "
        SELECT DISTINCT demo.number_of_adults_in_the_househol_437 AS household_members_18_and_over
        FROM civicrm_value_undocufund_in_57 demo
        WHERE 
          demo.entity_id = %0 AND 
          demo.number_of_adults_in_the_househol_437 IS NOT NULL
      ";
      $params = [ 0 => [ $contact_id, 'Integer' ] ];
      $dao = \CRM_Core_DAO::executeQuery($sql, $params);
      $dao->fetch();
      if (!isset($dao->household_members_18_and_over)) {;
        $missing_over_18++;
        $record['household_members_18_and_over'] = '';
      }
      else {
        $record['household_members_18_and_over'] = $dao->household_members_18_and_over;
      }

      // NUMBER UNDER 18
      $sql = "
        SELECT DISTINCT demo.number_of_children_in_the_househ_438 AS household_members_under_18
        FROM civicrm_value_undocufund_in_57 demo
        WHERE 
          demo.entity_id = %0 AND 
          demo.number_of_children_in_the_househ_438 IS NOT NULL
      ";
      $params = [ 0 => [ $contact_id, 'Integer' ] ];
      $dao = \CRM_Core_DAO::executeQuery($sql, $params);
      $dao->fetch();

      if (!isset($dao->household_members_under_18)) {
        $missing_under_18++;
        $record['household_members_under_18'] = '';
      }
      else {
        $record['household_members_under_18'] = $dao->household_members_under_18;
      }

      // PRIMARY SOURCE OF INCOME
      $sql = "
        SELECT DISTINCT survey.employment_industry_type_of_work_528 as primary_source_of_income
        FROM civicrm_activity_contact ac JOIN 
          civicrm_value_undocufund_on_58 survey ON ac.activity_id = survey.entity_id
        WHERE 
          ac.contact_id = %0 AND 
          survey.employment_industry_type_of_work_528 IS NOT NULL
      ";

      $params = [ 0 => [ $contact_id, 'Integer' ] ];
      $dao = \CRM_Core_DAO::executeQuery($sql, $params);
      $primary_source_of_income = NULL;
      while ($dao->fetch()) {
        if (isset($dao->primary_source_of_income)) {
          $primary_source_of_income_ids = explode(CRM_Core_DAO::VALUE_SEPARATOR,$dao->primary_source_of_income);
          $primary_source_of_incomes = [];
          foreach ($primary_source_of_income_ids as $primary_source_of_income_id) {
            $raw_value = trim($primary_source_of_income_id);
            if (empty($raw_value)) {
              continue;
            }
            $primary_source_of_incomes[] = $primary_source_of_income_options[$raw_value];
          }
          $primary_source_of_income = implode(",",$primary_source_of_incomes);
          break;
        }
      }
      if (!$primary_source_of_income) {
        $missing_primary_source_of_income++;
      }
      $record['primary_source_of_income'] = $primary_source_of_income;

      // GREATEST NEED 
      $sql = "
        SELECT DISTINCT survey.use_of_funds_greatest_need_585 AS greatest_need 
        FROM civicrm_activity_contact ac JOIN 
          civicrm_value_undocufund_on_58 survey ON ac.activity_id = survey.entity_id
        WHERE 
          record_type_id = 3 AND 
          ac.contact_id = %0 AND 
          survey.use_of_funds_greatest_need_585 IS NOT NULL
      ";

      $params = [ 0 => [ $contact_id, 'Integer' ] ];
      $dao = \CRM_Core_DAO::executeQuery($sql, $params);
      $greatest_need = NULL;
      while($dao->fetch()) {
        if ($dao->greatest_need) {
          $greatest_need_ids = explode(CRM_Core_DAO::VALUE_SEPARATOR,$dao->greatest_need);
          $greatest_needs = [];
          foreach ($greatest_need_ids as $greatest_need_id) {
            $raw_value = trim($greatest_need_id);
            if (empty($raw_value)) {
              continue;
            }
            $greatest_needs[] = $greatest_need_options[$raw_value];
          }
          if (count($greatest_needs) > 0) {
            $greatest_need = implode(",",$greatest_needs);
          }
        }
      }
      if (!$greatest_need) {
        $missing_greatest_need++;
      }
      $record['greatest_need'] = $greatest_need;
      $result[] = $record; 
    }
    if ($csv == 0) {
      print_r($result);
      echo "Count: " . count($result) . "\n";
      echo "Missing Gender: " . $missing_gender . "\n";
      echo "Missing Race: " . $missing_race . "\n";
      echo "Missing Under 18: " . $missing_under_18 . "\n";
      echo "Missing Over 18: " . $missing_over_18 . "\n";
      echo "Missing Greatest Need: " . $missing_greatest_need . "\n";
      echo "Missing Primary source of income: " . $missing_primary_source_of_income . "\n";
    }
    else {
      $out = fopen('php://output', 'w');
      $header = array_keys($result[0]);
      fputcsv($out, $header);
      foreach($result as $row) {
        fputcsv($out, $row);
      }
      fclose($out);
    }
}


?>
