<?php
/**
 * @file
 * Rules integration for inline conditions module.
 */

/**
 * Implements hook_rules_condition_info().
 *
 * @return array
 *   A list of rule conditions.
 */
function inline_conditions_rules_condition_info() {
  $items = array();

  $items['inline_conditions_general_condition'] = array(
    'label'     => t('Check inline conditions'),
    'group'     => t('Inline Conditions'),
    'parameter' => array(
      'entity' => array(
        'label'   => t('Entity'),
        'type'    => 'entity',
        'wrapped' => TRUE,
      ),
      'condition_settings' => array(
        'label'     => t('Condition settings values serialized'),
        'type'      => 'text',
      ),
    ),
    'base'      => 'inline_conditions_general_condition',
  );

  return $items;
}


/**
 * Rules callback: check conditions provided by a inline conditions field.
 *
 * @param object $wrapper
 *   A wrapped entity type given by rules.
 * @param array $condition_settings
 *   Serialized entity->field['parent_delta'].
 *
 * @return boolean
 *   Return True if condition is valid. False otherwise.
 */
function inline_conditions_general_condition($wrapper, $condition_settings) {
  // Fetch conditions info.
  $conditions = inline_conditions_get_info();

  if (is_string($condition_settings)) {
    $condition_settings = unserialize($condition_settings);
  }

  foreach ($condition_settings as $delta => $condition) {
    // Get the build callback of the condition.
    $callback = $conditions[$condition['condition_name']]['callbacks']['build'];

    // If sub condition returns FALSE, return FALSE for the main condition.
    if (!$callback($wrapper, $condition['condition_settings'])) {
      return FALSE;
    }
  }

  // Return TRUE if ALL sub conditions return TRUE.
  return TRUE;
}
