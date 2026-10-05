<?php
/**
 * @file
 * Views integration for Field Change Log.
 */

/**
 * Implements hook_views_data().
 *
 * For each tracked field, the entity's base table gets a "last changed" date
 * (field, sort and filter) and a relationship to the person who changed it.
 * Each tracked field is joined separately, filtered to that field, so several
 * tracked fields can be used in the same View without duplicate rows.
 */
function field_change_log_views_data() {
  $data = [];

  foreach (field_change_log_tracked_fields() as $entity_type => $bundles) {
    $info = entity_get_info($entity_type);
    if (empty($info['base table']) || empty($info['entity keys']['id'])) {
      continue;
    }

    // Collect each tracked field once per entity type, with a label.
    $labels = [];
    foreach ($bundles as $bundle => $field_names) {
      foreach ($field_names as $field_name) {
        if (!isset($labels[$field_name]) && ($instance = field_info_instance($entity_type, $field_name, $bundle))) {
          $labels[$field_name] = $instance['label'];
        }
      }
    }

    foreach ($labels as $field_name => $label) {
      $table = 'field_change_log__' . $entity_type . '__' . $field_name;
      $vars = ['@field' => $label, '@name' => $field_name];

      $data[$table]['table'] = [
        'group' => t('Field Change Log'),
        'join' => [
          $info['base table'] => [
            'table' => 'field_change_log_latest',
            'left_field' => $info['entity keys']['id'],
            'field' => 'entity_id',
            'extra' => [
              ['field' => 'entity_type', 'value' => $entity_type],
              ['field' => 'field_name', 'value' => $field_name],
            ],
          ],
        ],
      ];

      $data[$table]['changed'] = [
        'title' => t('@field last changed', $vars),
        'help' => t('When @field (@name) last changed. Empty if no change has been recorded.', $vars),
        'field' => [
          'handler' => 'views_handler_field_date',
          'click sortable' => TRUE,
        ],
        'sort' => [
          'handler' => 'views_handler_sort_date',
        ],
        'filter' => [
          'handler' => 'views_handler_filter_date',
        ],
      ];

      $data[$table]['uid'] = [
        'title' => t('@field last changed by', $vars),
        'help' => t('The person who last changed @field (@name).', $vars),
        'relationship' => [
          'handler' => 'views_handler_relationship',
          'base' => 'users',
          'base field' => 'uid',
          'label' => t('@field changed by', $vars),
        ],
      ];
    }
  }

  return $data;
}
