-- Create a sql file that can be imported back
SELECT CONCAT("INSERT INTO civicrm_managed SET id = ", id, ",module='", module, "', name='", name, "', entity_type='", entity_type, "', entity_id=", entity_id, ";") FROM civicrm_managed WHERE entity_type = 'PaymentProcessorType' OR entity_type = 'ContactType';
-- Now delete
DELETE FROM civicrm_managed WHERE entity_type = 'PaymentProcessorType';


