<?php

/**
 * Control which content types get their geofields sync'd.
 */
function sync_postgis_admin_settings() {
  $instances = field_info_instances('node');
  $form = array();
  foreach ($instances as $bundle_name => $fields) {
    $options = array();
    foreach ($fields as $name => $info) {
      if (($info['widget']['module'] == 'geofield') || ($info['widget']['module'] == 'geocoder')) {
        $options[$name] = $info['label'];
      }
    }
    if (!empty($options)) {
      $form['sync_postgis_' . $bundle_name] = array(
        '#markup' => '<h2>' . $bundle_name . '</h2>',
      );
      $form['sync_postgis_enabled_' . $bundle_name] = array(
        '#type' => 'checkboxes',
        '#title' => t('Enable PostGIS syncing for these fields'),
        '#options' => $options,
        '#default_value' => variable_get('sync_postgis_enabled_' . $bundle_name, array()),
        '#description' => t('Check which fields you would like sync\'d to PostGIS for this content type'),
      );
    }
  }

  return system_settings_form($form);
}

/**
 * Bulk sync form.
 */
function sync_postgis_bulk_sync($form, &$form_state) {
  $form = array();
  $form['bulk_sync'] = array(
    '#type' => 'markup',
    '#markup' => '<div>' . t('Clicking the button below will cause all exiting content with geofields configured for syncing to be syncd to the PostGIS db.') . '</div>',
  );
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Sync all PostGis content'),
  );
  return $form;
}

/**
 * Bulk sync form.
 */
function sync_postgis_bulk_sync_submit($form, &$form_state) {
  $instances = field_info_instances('node');
  $nodes = array();
  $queue = DrupalQueue::get('sync_postgis_queue');
  foreach ($instances as $bundle_name => $fields) {
    $enabled = array_filter(variable_get('sync_postgis_enabled_' . $bundle_name, array()));
    if (!empty($enabled)) {
      // Add all nodes of this type for processing.
      $query = db_select('node', 'n', array('fetch' => PDO::FETCH_ASSOC));
      $query->fields('n', array('nid', 'title'));
      $query->condition('n.type', $bundle_name);
      try {
        $result = $query->execute();
        foreach ($result as $row) {
          $queue->createItem($row);
        }
      }
      catch (Exception $e) {
        watchdog_exception('sync_postgis_bulk', $e);
      }

    }
  }
  drupal_set_message(t('All relevant content has been queued for syncing to the PostGIS db.'));
}

/**
 * Ensure we can connect to PostGIS and our table exists.
 */
function sync_postgis_check_db_form($form, &$form_state) {
  db_set_active(SYNC_POSTGIS_DB);
  if (!db_table_exists('node')) {
    $form['create_table'] = array(
      '#type' => 'submit',
      '#value' => 'Create PostGis Node table',
    );
    db_set_active();
  }
  else {
    db_set_active();
    drupal_set_message('The node table already exists in your postgis database');
  }
  return $form;
}

//function sync_postgis_check_db_form_submit($form, &$form_state) {
//  db_set_active(SYNC_POSTGIS_DB);
//  try {
//    if (!db_table_exists('node')) {
//      // create it
//      global $databases;
//      $pg_db = $databases[SYNC_POSTGIS_DB]['default']['database'];
//      $cmd = sprintf('psql -d %s -f %s', $pg_db, drupal_get_path('module', 'sync_postgis') . '/sync_postgis.postgres.sql');
//      // TODO: Figure out how to make this work!!
//      $output = shell_exec($cmd);
//    }
//  }
//  catch (Exception $e) {
//    watchdog_exception('sync_postgis', $e);
//  }
//  db_set_active();
//}