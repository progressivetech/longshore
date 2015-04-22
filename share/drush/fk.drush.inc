<?php

/**
 * CiviCRM foreign key management.
 * 
 * Functions used to ensure all CiviCRM foreign keys are set properly.
 */

/**
 * Implements drush_hook_command.
 */
function fk_drush_command() {
  $items['fk-print'] = array(
    'description' => 'Outputs the foreign keys set in the database in json format',
  );
  $items['fk-delete-orphans'] = array(
    'description' => 'Delete or set to NULL all orphaned records',
  );
  $items['fk-fix'] = array(
    'description' => 'Updates database to have the same foreign keys as defined in the provided JSON file.',
    'options' => array(
      '--path' => 'The path to the JSON file with the defined foreign keys.'
    ),
  );
  return $items;
}

/**
 * Delete orphaned foreign key records.
 * Help from https://github.com/michaelirey/mysql-foreign-key-checker
 *
 * This function finds orphans (as defined by the existing schema) and 
 * either deletes them are sets them to NULL depending on the DELETE_RULE. 
 */
function drush_fk_delete_orphans() {
  _civicrm_init();
  $sql = "SELECT kcu.`TABLE_NAME`, kcu.`COLUMN_NAME`, kcu.`REFERENCED_TABLE_NAME`,
    kcu.`REFERENCED_COLUMN_NAME`, rc.`DELETE_RULE` FROM information_schema.KEY_COLUMN_USAGE kcu LEFT JOIN
    INFORMATION_SCHEMA.REFERENTIAL_CONSTRAINTS rc ON kcu.CONSTRAINT_NAME = rc.CONSTRAINT_NAME
    WHERE kcu.REFERENCED_TABLE_SCHEMA IS NOT NULL";
  $dao = CRM_Core_DAO::executeQuery($sql);
  while($dao->fetch()) {
    if($dao->TABLE_NAME == $dao->REFERENCED_TABLE_NAME) {
      $find_sql = "SELECT t1.id
        FROM $dao->TABLE_NAME t1 LEFT JOIN $dao->REFERENCED_TABLE_NAME t2 ON
        (t1.$dao->COLUMN_NAME = t2.$dao->REFERENCED_COLUMN_NAME)
        WHERE t1.$dao->COLUMN_NAME IS NOT NULL AND
         t2.$dao->REFERENCED_COLUMN_NAME IS NULL";
    }
    else {
      $find_sql = "SELECT $dao->TABLE_NAME.id
        FROM $dao->TABLE_NAME LEFT JOIN $dao->REFERENCED_TABLE_NAME ON
       ($dao->TABLE_NAME.$dao->COLUMN_NAME = $dao->REFERENCED_TABLE_NAME.$dao->REFERENCED_COLUMN_NAME)
       WHERE $dao->TABLE_NAME.$dao->COLUMN_NAME IS NOT NULL
       AND $dao->REFERENCED_TABLE_NAME.$dao->REFERENCED_COLUMN_NAME IS NULL";
    }
    $fk_dao = CRM_Core_DAO::executeQuery($find_sql);
    if($fk_dao->N > 0) {
      $msg = dt("Found " . $fk_dao->N .
        " Invalid Foreign Keys in table $dao->TABLE_NAME ($dao->REFERENCED_TABLE_NAME): " .
        $dao->DELETE_RULE, 'ok');
      drush_log($msg, 'ok');
      $sql = "CREATE TEMPORARY TABLE cruft (id int)";
      CRM_Core_DAO::executeQuery($sql);
      $sql = "INSERT INTO cruft $find_sql";
      CRM_Core_DAO::executeQuery($sql);
      if($dao->DELETE_RULE == 'CASCADE') {
        $sql = "DELETE FROM $dao->TABLE_NAME WHERE id IN (SELECT id FROM cruft)";
      }
      else {
        $sql = "UPDATE $dao->TABLE_NAME SET $dao->COLUMN_NAME = NULL WHERE id IN (SELECT id FROM cruft)";
      }
      CRM_Core_DAO::executeQuery($sql);
      $sql = "DROP TABLE cruft";
      CRM_Core_DAO::executeQuery($sql);
    }
  }
}

/**
 * Generate foreign key schema file in JSON format.
 * 
 * This function creates the output json file with the up-to-date schema.
 * It should be run against a brand new database. Save the output to
 * /var/aegir/.drush/provision_pb/foreign.key.constraints.json
 *
 * drush @schema.piglet php-eval 'provision_pb_generate_fk_standard()' > \
 * /var/aegir/.drush/provision_pb/foreign.key.constraints.json
 */
function drush_fk_print() {
  _civicrm_init();
  $ref = fk_get();
  echo json_encode($ref);
}

/**
 * Output foreign key references as an array.
 * 
 * Returns an array of all fk references.
 */
function fk_get() {
  _civicrm_init();

  $sql = "SELECT kcu.constraint_name, kcu.table_name, kcu.column_name,
    kcu.referenced_table_name, kcu.referenced_column_name, rc.update_rule,
    rc.delete_rule FROM information_schema.key_column_usage kcu
    JOIN information_schema.referential_constraints rc
    on kcu.constraint_name = rc.constraint_name 
    WHERE referenced_table_schema = database() AND 
    information_schema.kcu.table_name not like 'civicrm_value%'
    ORDER BY constraint_name";
  $dao = CRM_Core_DAO::executeQuery($sql);
  $reference = array();
  while($dao->fetch()) {
    $table = $dao->table_name;
    $constraint_name = $dao->constraint_name;
    $reference[$table][$constraint_name] = array(
      'referenced_table_name' => $dao->referenced_table_name,
      'column_name' => $dao->column_name,
      'referenced_column_name' => $dao->referenced_column_name,
      'update_rule' => $dao->update_rule,
      'delete_rule' => $dao->delete_rule,
    );
   }

  return $reference;
}

/**
 * Generate a sql statement to create a foreign key
 *
 * Given the passed table, constraint name and values, generate
 * an ALTER TABLE query that will add the given constraint.
 */
function fk_sql_for_fk($table, $constraint_name, $values) {
  $values = (array) $values;
  $referenced_table_name = $values['referenced_table_name'];
  $column_name = $values['column_name'];
  $referenced_column_name = $values['referenced_column_name'];
  $rule = array();
  if($values['delete_rule'] != 'RESTRICT') {
    $rule[] = "ON DELETE " . $values['delete_rule'];
  }
  if($values['update_rule'] != 'RESTRICT') {
    $rule[] = "ON UPDATE " . $values['delete_rule'];
  }
  $rule = trim(implode(' ', $rule));
  
  $sql = "ALTER TABLE `$table` ADD CONSTRAINT $constraint_name FOREIGN KEY
    (`$column_name`) REFERENCES `$referenced_table_name` (`$referenced_column_name`) $rule";
  return $sql;
}

/**
 * Check if a given KEY exists. 
 *
 * Return TRUE if it exists or FALSE otherwise.
 */
function fk_key_exists($table, $constraint_name) {
  // Check if we need to first drop an key. I can't find any way to search for this
  // information other than SHOW CREATE TABLE.
  $dao = CRM_Core_DAO::executeQuery("SHOW CREATE TABLE `$table`");
  $dao->fetch();
  $sql = '';
  if(preg_match('/KEY `' . $constraint_name . '`/', $dao->Create_Table)) {
    return TRUE;
  }
  return FALSE;
}

/**
 * Ensure all foreign keys are set
 *
 * Compare the current database with a reference set of foreign keys
 * and drop/add foreign keys as necessary.
 */
function drush_fk_fix() {
  _civicrm_init();
  // Pull in reference schema. This was created against civicrm using fk_print().
  $path = drush_get_option('path', NULL);
  if(!file_exists($path)) {
    drush_log(dt("Failed to find path to JSON file."), 'error');
    return FALSE;
  }
  $json = file_get_contents($path);
  $ref = json_decode($json); 
  $site = fk_get();

  $queries = array(); 
  // First iterate over existing foreign keys - and if anything is missing from the reference
  // site, then delete it.
  while(list($table, $constraints) = each($site)) {
    // If the table doesn't exist in the reference list, then continue. Different sites may have different extensions/modules
    // installed that create their own tables.
    if(!property_exists($ref, $table)) continue;
    while(list($constraint, $params) = each($constraints)) {
      $drop = FALSE;
      if(!property_exists($ref->$table, $constraint)) {
        $drop = TRUE;
      }
      else {
        // if it does exist, make sure all values are the same.
        if($params !== (array) $ref->$table->$constraint) {
          $drop = TRUE;
        }
      }
      if($drop) {
        $queries[] = "ALTER TABLE `$table` DROP FOREIGN KEY `$constraint`";
        // $column_name = $params['column_name'];
        // $queries[] = "ALTER TABLE `$table` DROP FOREIGN KEY `$column_name`";
      }
    }
  }

  while(list($table, $constraints) = each($ref)) {
    while(list($constraint, $params) = each($constraints)) {
      if(!array_key_exists($table, $site) || !array_key_exists($constraint, $site[$table])) {
        if(fk_key_exists($table, $constraint)) {
          // We have to drop this key first
          $queries[] = "ALTER TABLE `$table` DROP KEY `$constraint`";
        }
        // Special exception: this field seems to sometimes be set to NOT NULL
        // which causes the adding of the constraint to fail.
        if($constraint == 'FK_civicrm_financial_account_contact_id') {
          $queries[] = "ALTER TABLE `civicrm_financial_account` CHANGE COLUMN
            `contact_id` `contact_id` int(10) unsigned DEFAULT NULL COMMENT
            'FK to Contact ID that is responsible for the funds in this account'";
        }
        $queries[] = fk_sql_for_fk($table, $constraint, $params);
      }
    }
  } 
  CRM_Core_DAO::executeQuery("SET FOREIGN_KEY_CHECKS=0");
  while(list(,$query) = each($queries)) {
    drush_log($query, 'ok');
    if(!CRM_Core_DAO::executeQuery($query)) return;
  }
  CRM_Core_DAO::executeQuery("SET FOREIGN_KEY_CHECKS=1");
}
