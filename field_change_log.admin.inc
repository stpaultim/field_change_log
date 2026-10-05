<?php
/**
 * @file
 * Admin page for Field Change Log: tracked fields, and rebuilding change
 * dates from revision history.
 */

/**
 * Page callback: admin/config/content/field-change-log.
 */
function field_change_log_admin_page() {
  $build['intro'] = [
    '#markup' => '<p>' . t('To track a field, edit it under Manage Fields and check "Track when this field changes". Changes are recorded from then on.') . '</p>',
  ];

  $rows = [];
  foreach (field_change_log_tracked_fields() as $entity_type => $bundles) {
    $entity_info = entity_get_info($entity_type);
    foreach ($bundles as $bundle => $field_names) {
      foreach ($field_names as $field_name) {
        $instance = field_info_instance($entity_type, $field_name, $bundle);
        if (!$instance) {
          continue;
        }
        $stats = db_query('SELECT COUNT(*) AS changes, MAX(changed) AS latest FROM {field_change_log} WHERE entity_type = :type AND bundle = :bundle AND field_name = :field', [
          ':type' => $entity_type,
          ':bundle' => $bundle,
          ':field' => $field_name,
        ])->fetchObject();
        $label = check_plain($instance['label']) . ' <small>(' . check_plain($field_name) . ')</small>';
        if (module_exists('field_ui') && ($path = _field_ui_bundle_admin_path($entity_type, $bundle))) {
          $label = l($instance['label'], $path . '/fields/' . $field_name) . ' <small>(' . check_plain($field_name) . ')</small>';
        }
        $rows[] = [
          check_plain($entity_info['label'] ?? $entity_type),
          check_plain($entity_info['bundles'][$bundle]['label'] ?? $bundle),
          $label,
          $stats->changes,
          $stats->latest ? format_date($stats->latest, 'short') : t('None yet'),
        ];
      }
    }
  }
  $build['tracked'] = [
    '#theme' => 'table',
    '#header' => [t('Entity type'), t('Type'), t('Field'), t('Changes recorded'), t('Latest change')],
    '#rows' => $rows,
    '#empty' => t('No fields are tracked yet.'),
  ];

  if (field_change_log_tracked_fields('node')) {
    $build['rebuild'] = backdrop_get_form('field_change_log_rebuild_form');
  }
  return $build;
}

/**
 * Form: rebuild change dates from content revision history.
 */
function field_change_log_rebuild_form($form, &$form_state) {
  $form['rebuild'] = [
    '#type' => 'fieldset',
    '#title' => t('Fill in past changes from revision history'),
    '#description' => t('For content that has no recorded change yet, looks through its revisions and records each revision in which a tracked field changed. Content with recorded changes is left alone, so this is safe to run again. Only content (nodes) keeps revisions, and only content saved with new revisions has a reliable history: edits saved without a new revision keep the older revision\'s date.'),
  ];
  $form['rebuild']['use_updated_date'] = [
    '#type' => 'checkbox',
    '#title' => t('When there is no revision history, use the content\'s "updated" date'),
    '#description' => t('Content saved without revisions has no history to read. Checking this records its "updated" date for fields that have a value. That date is approximate: it may be later than the field\'s real last change.'),
  ];
  $form['rebuild']['submit'] = [
    '#type' => 'submit',
    '#value' => t('Fill in past changes'),
  ];
  return $form;
}

/**
 * Submit handler: start the rebuild batch.
 */
function field_change_log_rebuild_form_submit($form, &$form_state) {
  $operations = [];
  foreach (field_change_log_tracked_fields('node') as $bundle => $field_names) {
    foreach ($field_names as $field_name) {
      $operations[] = ['field_change_log_rebuild_operation', [$bundle, $field_name, !empty($form_state['values']['use_updated_date'])]];
    }
  }
  batch_set([
    'title' => t('Filling in past field changes'),
    'operations' => $operations,
    'finished' => 'field_change_log_rebuild_finished',
    'file' => backdrop_get_path('module', 'field_change_log') . '/field_change_log.admin.inc',
  ]);
}

/**
 * Batch operation: rebuild one tracked node field, 25 nodes per request.
 */
function field_change_log_rebuild_operation($bundle, $field_name, $use_updated_date, &$context) {
  if (!isset($context['sandbox']['nids'])) {
    $context['sandbox']['nids'] = db_query('SELECT nid FROM {node} WHERE type = :type ORDER BY nid', [':type' => $bundle])->fetchCol();
    $context['sandbox']['total'] = count($context['sandbox']['nids']);
    $context['results'] += ['history' => 0, 'approximate' => 0];
  }
  foreach (array_splice($context['sandbox']['nids'], 0, 25) as $nid) {
    $result = field_change_log_rebuild_node_field($nid, $bundle, $field_name, $use_updated_date);
    if ($result) {
      $context['results'][$result]++;
    }
  }
  $done = $context['sandbox']['total'] - count($context['sandbox']['nids']);
  $context['message'] = t('@field: @done of @total', ['@field' => $field_name, '@done' => $done, '@total' => $context['sandbox']['total']]);
  $context['finished'] = $context['sandbox']['total'] ? $done / $context['sandbox']['total'] : 1;
}

/**
 * Rebuilds the change log of one field of one node from its revisions.
 *
 * @return string|null
 *   'history' if changes were found in revisions, 'approximate' if the
 *   "updated" date was used, or NULL if nothing was recorded.
 */
function field_change_log_rebuild_node_field($nid, $bundle, $field_name, $use_updated_date) {
  // Never overwrite changes recorded as they happened.
  $existing = db_query('SELECT 1 FROM {field_change_log} WHERE entity_type = :type AND entity_id = :id AND field_name = :field', [
    ':type' => 'node',
    ':id' => $nid,
    ':field' => $field_name,
  ])->fetchField();
  if ($existing) {
    return NULL;
  }

  $field = field_info_field($field_name);
  $revisions = db_query('SELECT vid, timestamp, uid FROM {node_revision} WHERE nid = :nid ORDER BY vid', [':nid' => $nid])->fetchAllAssoc('vid');

  // A revision's timestamp is set only when that revision is created; saving
  // without a new revision updates its values in place but keeps the old
  // timestamp. So a single revision says nothing about when this field
  // changed, and changes found in the newest revision may really be later
  // than its date. History needs at least two revisions.
  if (count($revisions) > 1 && $field['storage']['type'] == 'field_sql_storage') {
    $values = field_change_log_revision_values($field, $nid);
    $previous = [];
    $found = FALSE;
    foreach ($revisions as $vid => $revision) {
      $current = field_change_log_normalize_values($field, $values[$vid] ?? []);
      foreach (array_unique(array_merge(array_keys($current), array_keys($previous))) as $langcode) {
        if (($current[$langcode] ?? []) !== ($previous[$langcode] ?? [])) {
          field_change_log_record('node', $bundle, $nid, $field_name, $langcode, (int) $revision->timestamp, (int) $revision->uid, $vid);
          $found = TRUE;
        }
      }
      $previous = $current;
    }
    if ($found) {
      return 'history';
    }
  }

  if ($use_updated_date) {
    $node = node_load($nid);
    $values = $node ? field_change_log_normalize_values($field, (array) ($node->{$field_name} ?? [])) : [];
    foreach (array_keys($values) as $langcode) {
      field_change_log_record('node', $bundle, $nid, $field_name, $langcode, (int) $node->changed, 0, $node->vid);
    }
    return $values ? 'approximate' : NULL;
  }
  return NULL;
}

/**
 * Loads every revision's values of a field for one node, from SQL storage.
 *
 * @return array
 *   [revision_id => [langcode => [delta => item]]].
 */
function field_change_log_revision_values(array $field, $nid) {
  $table = _field_sql_storage_revision_tablename($field);
  $values = [];
  $result = db_select($table, 'r')
    ->fields('r')
    ->condition('entity_type', 'node')
    ->condition('entity_id', $nid)
    ->condition('deleted', 0)
    ->orderBy('revision_id')
    ->orderBy('delta')
    ->execute();
  foreach ($result as $row) {
    $item = [];
    foreach (array_keys($field['columns']) as $column) {
      $item[$column] = $row->{_field_sql_storage_columnname($field['field_name'], $column)};
    }
    // Field storage tables call the language column "language".
    $values[$row->revision_id][$row->language][$row->delta] = $item;
  }
  return $values;
}

/**
 * Batch finished callback for the rebuild.
 */
function field_change_log_rebuild_finished($success, $results, $operations) {
  if (!$success) {
    backdrop_set_message(t('Filling in past changes stopped with an error. Run it again to continue; content already done is skipped.'), 'error');
    return;
  }
  backdrop_set_message(t('Filled in past changes: @history from revision history, @approximate from the "updated" date.', [
    '@history' => format_plural($results['history'] ?? 0, '1 field', '@count fields'),
    '@approximate' => format_plural($results['approximate'] ?? 0, '1 field', '@count fields'),
  ]));
}
