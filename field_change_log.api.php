<?php
/**
 * @file
 * API documentation for Field Change Log.
 *
 * Field Change Log defines no hooks. Other modules can use these functions:
 *
 * - field_change_log_get_latest($entity_type, $entity_id, $field_name,
 *   $langcode = NULL): the most recent change to a field, with ->changed
 *   (timestamp), ->uid and ->revision_id.
 * - field_change_log_tracked_fields($entity_type = NULL, $bundle = NULL):
 *   which fields are tracked.
 * - field_change_log_set_tracked($entity_type, $bundle, $field_name, $track):
 *   start or stop tracking a field, e.g. from an install hook.
 * - field_change_log_record(...): record a change directly, e.g. when
 *   importing content whose real change dates are known.
 */
