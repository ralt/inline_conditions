<?php
/**
 * @file
 * Provides Rules and Inline Conditions integration for commerce_order module.
 *
 * @requirements
 * commerce_order, commerce_price.
 */

/**
 * Implements hook_inline_conditions_info().
 */
function commerce_order_inline_conditions_info() {
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

  $conditions['inline_conditions_order_product'] = array(
    'label'       => t('Product'),
    'entity type' => 'commerce_order',
    'callbacks'   => array(
      'configure' => 'inline_conditions_order_product_configure',
      'build'     => 'inline_conditions_order_product_build',
    ),
  );

  $conditions['inline_conditions_order_product_quantity'] = array(
    'label'       => t('Product and quantity'),
    'entity type' => 'commerce_order',
    'callbacks'   => array(
      'configure' => 'inline_conditions_order_product_quantity_configure',
      'build'     => 'inline_conditions_order_product_quantity_build',
    ),
  );

  if (module_exists('commerce_shipping')) {
    $conditions['inline_conditions_shipping_method'] = array(
      'label'       => t('Shipping method'),
      'entity type' => 'commerce_order',
      'callbacks'   => array(
        'configure' => 'inline_conditions_shipping_method_configure',
        'build'     => 'inline_conditions_shipping_method_build',
      ),
    );
  }

  if (module_exists('taxonomy')) {
    $conditions['inline_conditions_order_taxonomy'] = array(
      'label'       => t('Product category'),
      'entity type' => 'commerce_order',
      'callbacks'   => array(
        'configure' => 'inline_conditions_order_taxonomy_configure',
        'build'     => 'inline_conditions_order_taxonomy_build',
      ),
    );
  }

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
 *   Returns True if condition is valid. False otherwise.
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
 * Build callback for inline_conditions_order_product.
 *
 * @param EntityDrupalWrapper $wrapper
 *   The wrapped entity given by the rule
 * @param array $settings
 *   An array of rule condition settings.
 *
 * @return bool
 *   Returns True if condition is valid. False otherwise.
 */
function inline_conditions_order_product_build(EntityDrupalWrapper $wrapper, $settings) {
  if ($wrapper->type() == 'commerce_order') {
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
function inline_conditions_order_product_quantity_configure($settings) {
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
function inline_conditions_order_product_quantity_build(EntityDrupalWrapper $wrapper, $settings) {
  if ($wrapper->type() == 'commerce_order') {
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

    // If the array is empty, every product are in the order, so returns TRUE.
    if (empty($product_ids)) {
      return TRUE;
    }
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

/**
 * Configuration callback for inline_conditions_taxonomy on order.
 *
 * @param array $settings
 *   Values for the form element.
 *
 * @return array
 *   Return a form element.
 */
function inline_conditions_order_taxonomy_configure($settings) {
  $form = array();

  $default_value = '';
  if (!empty($settings['taxonomy']['terms'])) {
    foreach ($settings['taxonomy']['terms'] as $delta => $term) {
      $default_value .= taxonomy_term_load($term['target_id'])->name . ' (' . $term['target_id'] . ')';
      if (($delta + 1) < count($settings['taxonomy']['terms'])) {
        $default_value .= ', ';
      }
    }
  }

  $form['taxonomy'] = array(
    '#type'       => 'container',
    '#attributes' => array('class' => array('product-taxonomy')),
  );

  $form['taxonomy']['terms'] = array(
    '#type'              => 'textfield',
    '#title'             => t('Terms'),
    '#title_display'     => 'invisible',
    '#required'          => TRUE,
    '#default_value'     => $default_value,
    '#autocomplete_path' => 'inline_conditions/autocomplete/taxonomy_term/1/0',
    '#element_validate'  => array('_inline_conditions_autocomplete_validate'),
  );

  $options = array(
    0 => t('All selected products must be in the shopping cart to activate the discount.'),
    1 => t('At least one of these products has to be in the shopping cart to activate the discount.'),
  );

  $form['taxonomy']['config'] = array(
    '#type'          => 'radios',
    '#title'         => t('Config'),
    '#title_display' => 'invisible',
    '#default_value' => isset($settings['taxonomy']['config']) ? $settings['taxonomy']['config'] : 1,
    '#options'       => $options,
  );

  return $form;
}

/**
 * Build callback for inline_conditions_taxonomy on order.
 *
 * @param EntityDrupalWrapper $wrapper
 *   Wrapped entity type given by the rule.
 * @param array $settings
 *   Values for the condition settings.
 *
 * @return boolean
 *   True is sub condition is valid. FALSE otherwise.
 */
function inline_conditions_order_taxonomy_build(EntityDrupalWrapper $wrapper, $settings) {

  if ($wrapper->type() == 'commerce_order') {
    // Get terms ids from $settings.
    $term_ids = array();
    foreach ($settings['taxonomy']['terms'] as $delta => $term) {
      $term_ids[$term['target_id']] = $term['target_id'];
    }

    // Get checkboxes values.
    $one_product_checkbox = ($settings['taxonomy']['config'] == 1) ? TRUE : FALSE;
    $all_products_checkbox = ($settings['taxonomy']['config'] == 0) ? TRUE : FALSE;

    // Compare discount product ids with thoses of given order.
    foreach ($wrapper->commerce_line_items->value() as $line_item) {
      // Ensures that the type of current line item is product.
      if ($line_item->type == 'product') {
        // Wrap line item data.
        $wrapper_line_item = entity_metadata_wrapper('commerce_line_item', $line_item);

        // Get the product.
        $product = $wrapper_line_item->commerce_product->value();

        // Get properties of commerce_product entity type.
        $product_properties = entity_get_property_info('commerce_product');

        // Initialize an array storing field name of term reference type
        // field for the current product type.
        $term_fields = array();
        // Fetch all the fields name of taxonomy_term type for the current
        // product type.
        foreach ($product_properties['bundles'][$product->type]['properties'] as $field_name => $field_value) {
          if (preg_match('/taxonomy_term/', $field_value['type'])) {
            $term_fields[] = $field_name;
          }
        }

        foreach ($term_fields as $delta => $field_term_name) {
          // Get the value of the taxonomy term field.
          $field_term_value = field_get_items('commerce_product', $product, $field_term_name);
          if (!empty($field_term_value)) {
            // Unset term id if found into product taxonomy field.
            foreach ($field_term_value as $delta => $term_id) {
              if (array_key_exists($term_id['tid'], $term_ids)) {
                unset($term_ids[$term_id['tid']]);
              }
            }

            // If doesn't match every terms and all products have to be checked.
            if (!empty($term_ids) && $all_products_checkbox) {
              return FALSE;
            }
            // If term_ids is empty and one product have to be checked at leeat.
            elseif (empty($term_ids) && $one_product_checkbox) {
              return TRUE;
            }
          }
          // The product taxonomy term field is empty, return FALSE.
          else {
            return FALSE;
          }
          // Unset field term values.
          unset($field_term_value);
        }
      }
    }
    return TRUE;
  }
  return FALSE;
}