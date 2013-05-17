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

  $conditions['commerce_product_contains_specific_product'] = array(
    'label' => t('Product'),
    'entity type' => 'commerce_line_item',
    'callbacks' => array(
      'configure' => 'commerce_product_contains_specific_product_configure',
      'build' => 'commerce_product_contains_specific_product_build',
    ),
  );

  if (module_exists('taxonomy')) {
    $conditions['commerce_product_has_specified_terms'] = array(
      'label' => t('Taxonomy term(s)'),
      'entity type' => 'commerce_line_item',
      'callbacks' => array(
        'configure' => 'commerce_product_has_specified_terms_configure',
        'build' => 'commerce_product_has_specified_terms_build',
      ),
    );
  }

  return $conditions;
}

/**
 * Implements hook_inline_conditions_build_alter().
 */
function commerce_product_inline_conditions_build_alter(&$value) {
  switch ($value['condition_name']) {
    case 'commerce_product_has_specified_terms':
      $terms = $value['condition_settings']['terms'];

      $value['condition_settings']['terms'] = '';
      foreach ($terms as $term) {
        $value['condition_settings']['terms'] .= reset($term);
        if ($term !== end($terms)) {
          $value['condition_settings']['terms'] .= ', ';
        }
      }
      break;
    case 'commerce_product_contains_specific_product':
      if($product = commerce_product_load($value['condition_settings']['sku'][0]['target_id'])) {
        $value['condition_settings']['sku'] = $product->sku;
      }
      break;
  }
}

/**
 * Configuration callback for commerce_product_contains_specific_product.
 *
 * @param array $settings
 *   Values for the form element.
 *
 * @return array
 *   Return a form element.
 */
function commerce_product_contains_specific_product_configure($settings) {
  // Check if a product_id is found in $settings var.
  $default_value = '';
  if (isset($settings['sku'][0]['target_id'])) {
    $product = commerce_product_load($settings['sku'][0]['target_id']);
    $default_value = $product->title . ' (' . $product->product_id . ')';
  }

  $form = array();

  $form['sku'] = array(
    '#type' => 'textfield',
    '#title' => t('SKU'),
    '#title_display' => 'invisible',
    '#default_value' => $default_value,
    '#required' => TRUE,
    '#autocomplete_path' => 'inline_conditions/autocomplete/commerce_product/0/1',
    '#element_validate' => array('_inline_conditions_autocomplete_validate'),
    '#suffix' => '<div class="condition-instructions">' . t('The discount is active if the line item is a product with the selected SKU.') . '</div>',
  );

  return $form;
}

/**
 * Build callback for commerce_product_contains_specific_product.
 *
 * @param EntityDrupalWrapper $wrapper
 *   Wrapped entity type given by the rule.
 * @param array $sku
 *   Product sku returned by rule condition.
 *
 * @return boolean
 *   True if condition is valid. false otherwise.
 */
function commerce_product_contains_specific_product_build(EntityDrupalWrapper $wrapper, $sku) {
  if ($wrapper->getBundle() == 'product') {
    return ($wrapper->commerce_product->sku->value() == $sku);
  }

  return FALSE;
}

/**
 * Configuration callback for commerce_product_has_specified_terms on product.
 *
 * @param array $settings
 *   Values for the form element.
 *
 * @return array
 *   Return a form element.
 */
function commerce_product_has_specified_terms_configure($settings) {
  $form = array();

  $default_value = '';
  if (!empty($settings['terms'])) {
    foreach ($settings['terms'] as $delta => $term) {
      $default_value .= taxonomy_term_load($term['target_id'])->name . ' (' . $term['target_id'] . ')';
      if ($term != end($settings['terms'])) {
        $default_value .= ', ';
      }
    }
  }

  $form['terms'] = array(
    '#type' => 'textfield',
    '#title' => t('Terms'),
    '#title_display' => 'invisible',
    '#required' => TRUE,
    '#default_value' => $default_value,
    '#autocomplete_path' => 'inline_conditions/autocomplete/taxonomy_term/1/0',
    '#element_validate' => array('_inline_conditions_autocomplete_validate'),
    '#suffix' => '<div class="condition-instructions">' . t('The discount is active if the product has the selected term(s).') . '</div>',
  );

  return $form;
}

/**
 * Build callback for commerce_product_has_specified_terms on product.
 *
 * @param EntityDrupalWrapper $wrapper
 *   Wrapped entity type given by the rule.
 * @param array $terms
 *   Values for the condition settings.
 *
 * @return boolean
 *   True is condition is valid. false otherwise.
 */
function commerce_product_has_specified_terms_build(EntityDrupalWrapper $wrapper, $terms) {
  $terms_name = explode(', ', $terms);

  if ($wrapper->getBundle() == 'product') {
    // Fetch all the fields name of taxonomy_term type for the current
    // product type.
    $term_fields = array();
    foreach ($wrapper->commerce_product->getPropertyInfo() as $field_name => $property) {
      if (preg_match('/taxonomy_term/', $property['type'])) {
        $term_fields[] = $field_name;
      }
    }

    foreach ($term_fields as $term_field) {
      foreach ($wrapper->commerce_product->$term_field as $term) {
        if (($key = array_search($term->getIdentifier(), $terms_name)) !== FALSE) {
          unset($terms_name[$key]);
        }
      }
    }
  }

  return empty($terms_name);
}

/**
 * Implements hook_rules_condition_info_alter().
 *
 * Adds new rule conditions to commerce_line_item entity type.
 */
function commerce_product_rules_condition_info_alter(&$conditions) {
  $inline_conditions = inline_conditions_get_info();

  $conditions['commerce_product_contains_specific_product'] = array(
    'label' => t('Line item contains a specific product'),
    'parameter' => array(
      'commerce_line_item' => array(
        'type' => 'commerce_line_item',
        'label' => t('Line item'),
        'description' => t('The line item.'),
        'wrapped' => TRUE,
      ),
      'sku' => array(
        'type' => 'text',
        'label' => t('SKU'),
        'description' => t('Enter the product SKU to compare against the passed product line item.'),
      ),
    ),
    'module' => 'commerce_line_item',
    'group' => t('Commerce Line Item'),
    'callbacks' => array(
      'execute' => $inline_conditions['commerce_product_contains_specific_product']['callbacks']['build'],
    ),
  );

  $conditions['commerce_product_has_specified_terms'] = array(
    'label' => t('Line item contains specific terms ID'),
    'parameter' => array(
      'commerce_line_item' => array(
        'type' => 'commerce_line_item',
        'label' => t('Line item'),
        'description' => t('The line item.'),
        'wrapped' => TRUE,
      ),
      'terms' => array(
        'type' => 'text',
        'label' => t('Terms ID'),
        'description' => t('Enter a comma-separated list of term ID to compare against the passed product line item.'),
      ),
    ),
    'module' => 'commerce_line_item',
    'group' => t('Commerce Line Item'),
    'callbacks' => array(
      'execute' => $inline_conditions['commerce_product_has_specified_terms']['callbacks']['build'],
    ),
  );
}
