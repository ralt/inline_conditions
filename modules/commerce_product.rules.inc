<?php
/**
 * @file
 * Provides Rules and Inline Conditions integration for commerce_product module.
 */

/**
 * Implements hook_inline_condition_info().
 */
function commerce_product_inline_condition_info() {
  $conditions = array();

  $conditions['inline_conditions_order_product'] = array(
    'label'       => t('Product'),
    'entity type' => 'commerce_order',
    'callbacks'   => array(
      'configure' => 'inline_conditions_order_product_configure',
      'build'     => 'inline_conditions_product_build',
    ),
  );

  $conditions['inline_conditions_order_product_quantity'] = array(
    'label'       => t('Product and quantity'),
    'entity type' => 'commerce_order',
    'callbacks'   => array(
      'configure' => 'inline_conditions_product_quantity_configure',
      'build'     => 'inline_conditions_product_quantity_build',
    ),
  );

  $conditions['inline_conditions_product'] = array(
    'label'       => t('Product'),
    'entity type' => 'commerce_line_item',
    'callbacks'   => array(
      'configure' => 'inline_conditions_product_configure',
      'build'     => 'inline_conditions_product_build',
    ),
  );

  return $conditions;
}

/**
 * Configuration callback for inline_conditions_order_product.
 *
 * @param array $settings
 *   Values for the form element.
 *
 * @return array
 *   Return a form element.
 */
function inline_conditions_order_product_configure($settings) {
  // Get product IDs from $settings.
  $default_value = '';
  if (isset($settings['products'])) {
    foreach ($settings['products'] as $delta => $target_id) {
      $product = commerce_product_load($settings['products'][$delta]['target_id']);
      $default_value .= $product->title . ' (' . $product->product_id . ')';
      if (($delta + 1) < count($settings['products'])) {
        $default_value .= ', ';
      }
    }
  }

  $form = array();

  $form['products'] = array(
    '#type'              => 'textfield',
    '#title'             => t('SKUs'),
    '#title_display'     => 'invisible',
    '#default_value'     => $default_value,
    '#required'          => TRUE,
    '#autocomplete_path' => 'inline_conditions/autocomplete/commerce_product/0/0',
    '#element_validate'  => array('_inline_conditions_autocomplete_validate'),
    '#suffix'            => '<div class="condition-instructions">' . t('Select products when ordered make discount active.') . '</div>',
  );

  return $form;
}

/**
 * Configuration callback for inline_conditions_product.
 *
 * @param array $settings
 *   Values for the form element.
 *
 * @return array
 *   Return a form element.
 */
function inline_conditions_product_configure($settings) {
  $product_id = NULL;

  // Check if a product_id is found in $settings var.
  $default_value = '';
  if (isset($settings['products'][0]['target_id'])) {
    $product = commerce_product_load($settings['products'][0]['target_id']);
    $default_value = $product->title . ' (' . $product->product_id . ')';
  }

  $form = array();

  $form['products'] = array(
    '#type'              => 'textfield',
    '#title'             => t('SKUs'),
    '#title_display'     => 'invisible',
    '#default_value'     => $default_value,
    '#required'          => TRUE,
    '#autocomplete_path' => 'inline_conditions/autocomplete/commerce_product/0/1',
    '#element_validate'  => array('_inline_conditions_autocomplete_validate'),
  );

  return $form;
}

/**
 * Build callback for inline_conditions_product.
 *
 * @param EntityDrupalWrapper $wrapper
 *   Wrapped entity type given by the rule.
 * @param array $settings
 *   Values for the condition settings.
 *
 * @return boolean
 *   True is sub condition is valid. FALSE otherwise.
 */
function inline_conditions_product_build(EntityDrupalWrapper $wrapper, $settings) {

  switch ($wrapper->type()) {
    // Check if the given object is commerce_order type.
    case 'commerce_order':
      // Get an array of product ids from condition settings.
      $product_ids = array();
      foreach ($settings['products'] as $delta => $product) {
        $product_ids[$product['target_id']] = $product['target_id'];
      }

      // Compare discount product ids with thoses of given order.
      foreach ($wrapper->commerce_line_items->value() as $line_item) {
        // Ensures that the type of current line item is product.
        if ($line_item->type == 'product') {
          // Wrap line item data.
          $wrapper_line_item = entity_metadata_wrapper('commerce_line_item', $line_item);

          // Get product fully qualified object.
          $product = $wrapper_line_item->commerce_product->value();
          // Check if product ID is in the array.
          if (array_key_exists($product->product_id, $product_ids)) {
            unset($product_ids[$product->product_id]);
          }
        }
      }

      // If the array is empty, every product are in the order, so return TRUE.
      if (empty($product_ids)) {
        return TRUE;
      }
      break;

    // Check if the given object is commerce_line_item type.
    case 'commerce_line_item':
      // Get an array of product ids from condition settings.
      $product_ids = array();
      foreach ($settings['products'] as $delta => $product) {
        $product_ids[$product['target_id']] = $product['target_id'];
      }

      // Ensures that the type of current line item is product.
      if ($wrapper->getBundle() == 'product') {
        $product = $wrapper->commerce_product->value();
        // Check if product ID is in the array. If yes, return TRUE;
        if (array_key_exists($product->product_id, $product_ids)) {
          return TRUE;
        }
      }
      break;
  }
  return FALSE;
}

/**
 * Configure callback for inline_conditions_product_quantity.
 *
 * @param array $settings
 *   Values for the form element.
 *
 * @return array
 *   Return the form element to display.
 */
function inline_conditions_product_quantity_configure($settings) {
  // Set default values for form elements.
  $product_value = '';
  $min_quantity_value = '';
  $max_quantity_value = '';

  // Try to get product quantity.
  if (isset($settings['product_quantity']['products'])) {
    foreach ($settings['product_quantity']['products'] as $delta => $target_id) {
      $product = commerce_product_load($settings['product_quantity']['products'][$delta]['target_id']);
      $product_value .= $product->title . ' (' . $product->product_id . ')';
      if (($delta + 1) < count($settings['product_quantity']['products'])) {
        $product_value .= ', ';
      }
    }
  }
  if (isset($settings['product_quantity']['min_quantity']) && isset($settings['product_quantity']['max_quantity'])) {
    $min_quantity_value = $settings['product_quantity']['min_quantity'];
    $max_quantity_value = $settings['product_quantity']['max_quantity'];
  }

  $form = array();

  $form['product_quantity'] = array(
    '#type'   => 'container',
    '#suffix' => '<div class="condition-instructions">' . t('Select product(s) and quantity range to activate the current discount.') . '</div>',
  );

  $form['product_quantity']['products'] = array(
    '#type'              => 'textfield',
    '#title'             => t('product:'),
    '#default_value'     => $product_value,
    '#size'              => 40,
    '#required'          => TRUE,
    '#autocomplete_path' => 'inline_conditions/autocomplete/commerce_product/0/0',
    '#element_validate'  => array('_inline_conditions_autocomplete_validate'),
  );

  $form['product_quantity']['min_quantity'] = array(
    '#type'             => 'textfield',
    '#title'            => t('minimum quantity:'),
    '#default_value'    => $min_quantity_value,
    '#size'             => 5,
    '#required'         => TRUE,
    '#element_validate' => array('_inline_conditions_product_min_quantity_validate'),
  );

  $form['product_quantity']['max_quantity'] = array(
    '#type'             => 'textfield',
    '#title'            => t('and maximum quantity:'),
    '#default_value'    => $max_quantity_value,
    '#size'             => 5,
    '#required'         => FALSE,
    '#element_validate' => array('_inline_conditions_product_max_quantity_validate'),
  );

  return $form;
}

/**
 * Build callback for inline_conditions_product_quantity.
 *
 * @param EntityDrupalWrapper $wrapper
 *   Wrapped entity given by the rule.
 * @param array $settings
 *   Settings given by the rule.
 *
 * @return boolean
 *   True if the condition is valid. False otherwise.
 */
function inline_conditions_product_quantity_build(EntityDrupalWrapper $wrapper, $settings) {
  switch ($wrapper->type()) {
    // Check if the given object is commerce_order type.
    case 'commerce_order':
      // Get an array of product ids from condition settings.
      $product_ids = array();
      foreach ($settings['product_quantity']['products'] as $delta => $product) {
        $product_ids[$product['target_id']] = $product['target_id'];
      }

      // Get product quantity to check.
      $min_quantity = $settings['product_quantity']['min_quantity'];
      $max_quantity = $settings['product_quantity']['max_quantity'];

      // If max quantity has not been set.
      if (empty($max_quantity)) {
        // Compare discount product ids with thoses of given order.
        foreach ($wrapper->commerce_line_items->value() as $line_item) {
          // Ensures that the type of current line item is product.
          if ($line_item->type == 'product') {
            // Wrap line item data.
            $wrapper_line_item = entity_metadata_wrapper('commerce_line_item', $line_item);

            // Get product fully qualified object.
            $product = $wrapper_line_item->commerce_product->value();
            // Check if both product ID and quantity match.
            if (array_key_exists($product->product_id, $product_ids) && $min_quantity <= $line_item->quantity) {
              unset($product_ids[$product->product_id]);
            }
          }
        }
      }
      else {
        // Compare discount product ids with thoses of given order.
        foreach ($wrapper->commerce_line_items->value() as $line_item) {
          // Ensures that the type of current line item is product.
          if ($line_item->type == 'product') {
            // Wrap line item data.
            $wrapper_line_item = entity_metadata_wrapper('commerce_line_item', $line_item);

            // Get product fully qualified object.
            $product = $wrapper_line_item->commerce_product->value();
            // Check if both product ID and quantity match.
            if (array_key_exists($product->product_id, $product_ids) &&
              $min_quantity <= $line_item->quantity &&
              $max_quantity >= $line_item->quantity
            ) {
              unset($product_ids[$product->product_id]);
            }
          }
        }
      }

      // If the array is empty, every product are in the order, so return TRUE.
      if (empty($product_ids)) {
        return TRUE;
      }
      break;
  }
  return FALSE;
}

/**
 * Validation callback for max_quantity element.
 *
 * @param array $element
 *   The form element.
 * @param array $form_state
 *   The form state array.
 * @param array $form
 *   The form array.
 */
function _inline_conditions_product_max_quantity_validate($element, &$form_state, $form) {
  // Fetch form element value.
  $value = $element['#value'];

  if (!empty($value)) {
    // Get container of quantity form elements.
    $container = drupal_array_get_nested_value($form, array_slice($element['#array_parents'], 0, -1));

    // Get value of min_quantity form element.
    $min_quantity = (int) $container['min_quantity']['#value'];
    $max_quantity = (int) $value;

    if ($max_quantity <= $min_quantity) {
      form_set_error(implode('][', $element['#array_parents']), t('Maximum quantity must be greater than minimum quantity.'));
    }
  }
  elseif ($value === "0") {
    form_set_error(implode('][', $element['#array_parents']), t('Maximun quantity is an integer and greater than 0'));
  }
}


/**
 * Validation callback for min_quantity element.
 *
 * @param array $element
 *   The form element.
 * @param array $form_state
 *   The form state array.
 * @param array $form
 *   The form array.
 */
function _inline_conditions_product_min_quantity_validate($element, &$form_state, $form) {
  // Fetch form element value.
  $value = (int) $element['#value'];

  // Check if quantity value is an integer and greater than 0.
  if (!is_int($value) || $value <= 0) {
    form_set_error(implode('][', $element['#array_parents']), t('Mininimum quantity is an integer and greater than 0'));
  }
}
