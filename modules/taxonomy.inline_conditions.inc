<?php
/**
 * @file
 * Provides Rules and Inline Conditions integration for taxonomy module.
 */

/**
 * Implements hook_inline_conditions_info().
 *
 * This condition only check if the product has implemented a term
 * reference field.
 */
function taxonomy_inline_conditions_info() {
  $conditions = array();

  $conditions['inline_conditions_taxonomy'] = array(
    'label'       => t('Product category'),
    'entity type' => 'commerce_line_item',
    'callbacks'   => array(
      'configure' => 'inline_conditions_taxonomy_configure',
      'build'     => 'inline_conditions_taxonomy_build',
    ),
  );

  $conditions['inline_conditions_order_taxonomy'] = array(
    'label'       => t('Product category'),
    'entity type' => 'commerce_order',
    'callbacks'   => array(
      'configure' => 'inline_conditions_order_taxonomy_configure',
      'build'     => 'inline_conditions_order_taxonomy_build',
    ),
  );

  return $conditions;
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
function inline_conditions_taxonomy_configure($settings) {
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
    '#type' => 'container',
    '#attributes'       => array('class' => array('product-taxonomy')),
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
    '#type' => 'radios',
    '#title' => t('Config'),
    '#title_display' => 'invisible',
    '#default_value' => isset($settings['taxonomy']['config']) ? $settings['taxonomy']['config'] : 1,
    '#options' => $options,
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
function inline_conditions_taxonomy_build(EntityDrupalWrapper $wrapper, $settings) {

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
