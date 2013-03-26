<?php
/**
 * @file
 * Provides Rules and Inline Conditions integration for commerce_shipping
 * module.
 *
 * @requirements
 * commerce_shipping (>=7.X-2.0).
 */

/**
 * Implements hook_inline_conditions_info().
 */
function commerce_shipping_inline_conditions_info() {
  $conditions = array();

  $conditions['inline_conditions_shipping_method'] = array(
    'label'       => t('Shipping method'),
    'entity type' => 'commerce_order',
    'callbacks'   => array(
      'configure' => 'inline_conditions_shipping_method_configure',
      'build'     => 'inline_conditions_shipping_method_build',
    ),
  );

  return $conditions;
}

/**
 * Configuration callback for inline_conditions_shipping_method.
 *
 * @param array $settings
 *   An array of rules condition settings.
 *
 * @return array;
 *   A form element array.
 */
function inline_conditions_shipping_method_configure($settings) {
  $form = array();

  // Get shipping methods.
  $shipping_methods = commerce_shipping_methods();

  // Prepare options list for the form element.
  $options = array();
  if (!empty($shipping_methods)) {
    foreach ($shipping_methods as $name => $shipping_method) {
      $options[$name] = $shipping_method['title'];
    }
  }

  $default_value = FALSE;
  if (!empty($settings['shipping_method'])) {
    $default_value = $settings['shipping_method'];
  }

  $form['shipping_method'] = array(
    '#type'             => 'select',
    '#multiple'         => FALSE,
    '#options'          => $options,
    '#title'            => t('Shipping method'),
    '#title_display'    => 'invisible',
    '#default_value'    => $default_value,
    '#require'          => TRUE,
    '#element_validate' => array('_inline_conditions_shipping_method_validate'),
    '#suffix'           => '<div class="condition-instructions">' .
    t('The discount will be enable only if the shipping method is selected by customer.') .
    '</div>',
  );

  return $form;
}

/**
 * Validation callback for a inline_conditions_product autocomplete element.
 */
function _inline_conditions_shipping_method_validate($element, &$form_state, $form) {
  $shipping_methods = commerce_shipping_methods();

  if (!array_key_exists($element['#value'], $shipping_methods)) {
    form_set_error(implode('][', $element['#array_parents']), t('A correct shipping method must be selected'));
  }
}

/**
 * Build callback for inline_conditions_shipping_method.
 *
 * @param EntityDrupalWrapper $wrapper
 *   The wrapped entity given by the rule.
 * @param array $settings
 *   An array of rules condition settings.
 *
 * @return boolean
 *   return True if condition is valid. False otherwise.
 */
function inline_conditions_shipping_method_build(EntityDrupalWrapper $wrapper, $settings) {
  // Check if the given object is commerce_order type.
  if ($wrapper->type() == 'commerce_order') {

    // Get shipping method from $settings.
    $shipping_method = $settings['shipping_method'];

    foreach ($wrapper->commerce_line_items->value() as $line_item) {
      // Check if the line item contains a shipping service.
      if (array_key_exists('shipping_service', $line_item->data)) {
        $shipping_service = $line_item->data['shipping_service'];

        // If order shipping service matches condition shipping service, then
        // evaluate the condition at TRUE.
        if ($shipping_service['shipping_method'] == $shipping_method) {
          return TRUE;
        }
      }
    }
  }
  return FALSE;
}
