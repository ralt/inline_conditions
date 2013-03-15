<?php
/**
 * @file
 * Provides Rules and Inline Conditions integration for commerce_order module.
 *
 * @requirements
 * commerce_order, commerce_price.
 */

/**
 * Implements hook_inline_condition_info().
 */
function commerce_order_inline_condition_info() {
  $conditions = array();

  $conditions['inline_conditions_order_total'] = array(
    'label'       => t('Minimum order amount'),
    'entity type' => 'commerce_order',
    'callbacks'   => array(
      'configure' => 'inline_conditions_order_total_configure',
      'build'     => 'inline_conditions_order_total_build',
    ),
  );

  $conditions['inline_conditions_order_user'] = array(
    'label'       => t('User'),
    'entity type' => 'commerce_order',
    'callbacks'   => array(
      'configure' => 'inline_conditions_order_user_configure',
      'build'     => 'inline_conditions_order_user_build',
    ),
  );

  return $conditions;
}

/**
 * Configuration callback for inline_conditions_order_total.
 *
 * @param array $settings
 *   An array of rules condition settings.
 *
 * @return array;
 *   A form element array.
 */
function inline_conditions_order_total_configure($settings) {
  $form = array();

  // Get the default website currency.
  $default_currency = commerce_currency_load(NULL);

  $default_value = 0;
  if (!empty($settings)) {
    $default_value = commerce_currency_amount_to_decimal($settings['total']['amount'], $default_currency['code']);
  }

  $form['total'] = array(
    '#type'             => 'container',
    '#tree'             => TRUE,
    '#element_validate' => array('commerce_price_field_widget_validate'),
    '#suffix'           => '<div class="condition-instructions">' . t('Enter the minimum order amount to activate the discount.') . '</div>',
  );

  $form['total']['amount'] = array(
    '#type'          => 'textfield',
    '#title'         => t('Order total'),
    '#title_display' => 'invisible',
    '#default_value' => $default_value,
    '#size'          => 10,
    '#field_suffix'  => $default_currency['code'],
    '#require'       => TRUE,
  );

  $form['total']['currency_code'] = array(
    '#type'          => 'value',
    '#default_value' => $default_currency['code'],
  );

  return $form;
}

/**
 * Build callback for inline_conditions_order_total.
 *
 * @param EntityDrupalWrapper $wrapper
 *   The wrapped entity given by the rule.
 * @param array $settings
 *   An array of rule condition settings.
 *
 * @return boolean
 *   return True if condition is valid. False otherwise.
 */
function inline_conditions_order_total_build(EntityDrupalWrapper $wrapper, $settings) {
  // Check if the given wrapped entity is commerce_order type.
  if ($wrapper->type() == 'commerce_order') {
    // Get given total order amount.
    $total_order = $wrapper->commerce_order_total->value();

    // Check if a price component exists.
    if (!isset($total_order['amount']) || !isset($settings['total']['currency_code'])) {
      return FALSE;
    }

    // Ensures currency codes match.
    if ($settings['total']['currency_code'] != $total_order['currency_code']) {
      return FALSE;
    }

    // Determine whether the order total exceeds the specified amount.
    return $total_order['amount'] >= $settings['total']['amount'];
  }
  return FALSE;
}

/**
 * Configuration callback for inline_conditions_order_user.
 *
 * @param array $settings
 *   An array of rules condition settings.
 *
 * @return array;
 *   A form element array.
 */
function inline_conditions_order_user_configure($settings) {
  $form = array();

  $default_value = '';
  if (!empty($settings['user']['name'])) {
    $default_value = $settings['user']['name'];
  }

  $form['user'] = array(
    '#type'              => 'textfield',
    '#title'             => t('Users'),
    '#title_display'     => 'invisible',
    '#default_value'     => $default_value,
    '#required'          => TRUE,
    '#autocomplete_path' => 'user/autocomplete',
    '#element_validate'  => array('_inline_conditions_order_user_validate'),
    '#suffix'            => '<div class="condition-instructions">' . t('Select users allowed to use this discount.') . '</div>',
  );

  return $form;
}

/**
 * Validation callback for a inline_conditions_order_user autocomplete element.
 */
function _inline_conditions_order_user_validate($element, &$form_state, $form) {
  // Try to fetch user.
  $user = user_load_by_name($element['#value']);

  if (!empty($user)) {
    $value['uid'] = $user->uid;
    $value['name'] = $element['#value'];
    form_set_value($element, $value, $form_state);
  }
  else {
    form_set_error(implode('][', $element['#array_parents']), t('Enter a correct username.'));
  }
}

/**
 * Build callback for inline_conditions_order_user.
 *
 * @param EntityDrupalWrapper $wrapper
 *   The wrapped entity given by the rule.
 * @param array $settings
 *   An array of rule condition settings.
 *
 * @return boolean
 *   return True if condition is valid. False otherwise.
 */
function inline_conditions_order_user_build(EntityDrupalWrapper $wrapper, $settings) {
  switch ($wrapper->type()) {
    // Check if the given object is commerce_order type.
    case 'commerce_order':
      // Get user ID from $settings.
      $user_id = $settings['user']['uid'];

      // If current logged user matches the discount related users.
      return $user_id == $wrapper->uid->value();

    break;

    // Check if the given object is commerce_line_item type.
    case 'commerce_line_item':
      // Get user ID from $settings.
      $user_id = $settings['user']['uid'];
      // Get current logged user.
      global $user;
      // If current logged user matches the discount related users.
      return $user_id == $user->uid;

    break;
  }
  return FALSE;
}
