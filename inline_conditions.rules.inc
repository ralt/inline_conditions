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

  $conditions = inline_conditions_get_info();

  foreach ($conditions as $name => $condition) {
    $items[$name] = array(
      'label'     => $condition['label'],
      'group'     => t('Inline Conditions'),
      'parameter' => array(
        'entity' => array(
          'label'   => t('Entity'),
          'type'    => 'entity',
          'wrapped' => TRUE,
        ),
        'condition_settings' => array(
          'label'     => t('Serialized condition settings'),
          'type'      => 'text',
        ),
      ),
      'base'      => $condition['callbacks']['build'],
    );
  }

  return $items;
}
