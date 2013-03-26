<?php
/**
 * @file
 * Provides Rules and Inline Conditions integration for commerce_product module.
 */

/**
 * Implements hook_inline_conditions_info().
 */
function commerce_product_inline_conditions_info() {
  $conditions = array();

  $conditions['inline_conditions_product'] = array(
    'label'       => t('Product'),
    'entity type' => 'commerce_line_item',
    'callbacks'   => array(
      'configure' => 'inline_conditions_product_configure',
      'build'     => 'inline_conditions_product_build',
    ),
  );

  if (module_exists('taxonomy')) {
    $conditions['inline_conditions_taxonomy'] = array(
      'label'       => t('Product category'),
      'entity type' => 'commerce_line_item',
      'callbacks'   => array(
        'configure' => 'inline_conditions_product_taxonomy_configure',
        'build'     => 'inline_conditions_product_taxonomy_build',
      ),
    );
  }

  return $conditions;
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
 * Configuration callback for inline_conditions_taxonomy on product.
 *
 * @param array $settings
 *   Values for the form element.
 *
 * @return array
 *   Return a form element.
 */
function inline_conditions_product_taxonomy_configure($settings) {
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

  $form['taxonomy']['terms'] = array(
    '#type'              => 'textfield',
    '#title'             => t('Terms'),
    '#title_display'     => 'invisible',
    '#required'          => TRUE,
    '#default_value'     => $default_value,
    '#autocomplete_path' => 'inline_conditions/autocomplete/taxonomy_term/1/0',
    '#element_validate'  => array('_inline_conditions_autocomplete_validate'),
  );

  return $form;
}

/**
 * Build callback for inline_conditions_taxonomy on product.
 *
 * @param EntityDrupalWrapper $wrapper
 *   Wrapped entity type given by the rule.
 * @param array $settings
 *   Values for the condition settings.
 *
 * @return boolean
 *   True is sub condition is valid. FALSE otherwise.
 */
function inline_conditions_product_taxonomy_build(EntityDrupalWrapper $wrapper, $settings) {

  if ($wrapper->type() == 'commerce_line_item') {
    // Get terms ids from $settings.
    $term_ids = array();
    foreach ($settings['taxonomy']['terms'] as $delta => $term) {
      $term_ids[$term['target_id']] = $term['target_id'];
    }

    if ($wrapper->getBundle() == 'product') {
      // Get the product.
      $product = $wrapper->commerce_product->value();

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

          // If doesn't match every terms from rule settings, return FALSE.
          if (!empty($term_ids)) {
            return FALSE;
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
    return TRUE;
  }
  return FALSE;
}
