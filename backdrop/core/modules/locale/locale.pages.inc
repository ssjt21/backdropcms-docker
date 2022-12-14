<?php
/**
 * @file
 * Interface translation summary, editing and deletion user interfaces.
 */

/**
 * String search screen.
 */
function locale_translate_seek_screen() {
  // Add CSS.
  backdrop_add_css(backdrop_get_path('module', 'locale') . '/css/locale.css');

  $elements = backdrop_get_form('locale_translation_filter_form');
  $output = backdrop_render($elements);
  $output .= _locale_translate_seek();
  return $output;
}

/**
 * Perform a string search and display results in a table
 */
function _locale_translate_seek() {
  $output = '';

  // We have at least one criterion to match
  if (!($query = _locale_translate_seek_query())) {
    $query = array(
      'translation' => 'all',
      'language' => 'all',
      'string' => '',
    );
  }

  $sql_query = db_select('locales_source', 's');

  $limit_language = NULL;
  if ($query['language'] != 'en' && $query['language'] != 'all') {
    $sql_query->leftJoin('locales_target', 't', "t.lid = s.lid AND t.language = :langcode", array(':langcode' => $query['language']));
    $limit_language = $query['language'];
  }
  else {
    $sql_query->leftJoin('locales_target', 't', 't.lid = s.lid');
  }

  $sql_query->fields('s', array('source', 'location', 'context', 'lid'));
  $sql_query->fields('t', array('translation', 'language'));

  // Compute LIKE section.
  switch ($query['translation']) {
    case 'translated':
      $sql_query->condition('t.translation', '%' . db_like($query['string']) . '%', 'LIKE');
      $sql_query->orderBy('t.translation', 'DESC');
      break;
    case 'untranslated':
      $sql_query->condition(db_and()
        ->condition('s.source', '%' . db_like($query['string']) . '%', 'LIKE')
        ->isNull('t.translation')
      );
      $sql_query->orderBy('s.source');
      break;
    case 'all' :
    default:
      $condition = db_or()
        ->condition('s.source', '%' . db_like($query['string']) . '%', 'LIKE');
      if ($query['language'] != LANGUAGE_SYSTEM) {
        // Only search in translations if the language is not forced to system language.
        $condition->condition('t.translation', '%' . db_like($query['string']) . '%', 'LIKE');
      }
      $sql_query->condition($condition);
      break;
  }

  $sql_query = $sql_query->extend('PagerDefault')->limit(50);
  $locales = $sql_query->execute();

  $header = array(t('String'), t('Context'), ($limit_language) ? t('Language') : t('Languages'), array('data' => t('Operations'), 'colspan' => '2'));

  $strings = array();
  foreach ($locales as $locale) {
    if (!isset($strings[$locale->lid])) {
      $strings[$locale->lid] = array(
        'languages' => array(),
        'location' => $locale->location,
        'source' => $locale->source,
        'context' => $locale->context,
      );
    }
    if (isset($locale->language)) {
      $strings[$locale->lid]['languages'][$locale->language] = $locale->translation;
    }
  }

  $rows = array();
  foreach ($strings as $lid => $string) {
    $links = array();
    $links['edit'] = array(
      'title' => t('Edit'),
      'href' => "admin/config/regional/translate/edit/$lid",
      'query' => backdrop_get_destination(),
      'class' => array('nowrap'),
    );
    $links['delete'] = array(
      'title' => t('Delete'),
      'href' => "admin/config/regional/translate/delete/$lid",
      'query' => backdrop_get_destination(),
      'class' => array('nowrap'),
    );
    $rows[] = array(
      array('data' => check_plain(truncate_utf8($string['source'], 150, FALSE, TRUE)) . '<br /><small>' . $string['location'] . '</small>'),
      $string['context'],
      array('data' => _locale_translate_language_list($string, $limit_language), 'align' => 'center'),
      array('data' => array('#type' => 'operations', '#links' => $links)),
    );
  }

  $output .= theme('table', array('header' => $header, 'rows' => $rows, 'empty' => t('No strings available.')));
  $output .= theme('pager');

  return $output;
}

/**
 * List languages in search result table
 */
function _locale_translate_language_list($string, $limit_language) {
  // Add CSS.
  backdrop_add_css(backdrop_get_path('module', 'locale') . '/css/locale.css');

  // Include both translated and not yet translated target languages in the
  // list. The source language is English.
  $languages = language_list();
  if (!locale_translate_english()) {
    unset($languages['en']);
  }
  $output = '';
  foreach ($languages as $langcode => $language) {
    if (!$limit_language || $limit_language == $langcode) {
      $output .= (!empty($string['languages'][$langcode])) ? $langcode . ' ' : "<em class=\"locale-untranslated\">$langcode</em> ";
    }
  }

  return $output;
}

/**
 * Build array out of search criteria specified in request variables
 */
function _locale_translate_seek_query() {
  $query = &backdrop_static(__FUNCTION__);
  if (!isset($query)) {
    $query = array();
    $fields = array('string', 'language', 'translation');
    foreach ($fields as $field) {
      if (isset($_SESSION['locale_translation_filter'][$field])) {
        $query[$field] = $_SESSION['locale_translation_filter'][$field];
      }
    }
  }
  return $query;
}

/**
 * List locale translation filters that can be applied.
 */
function locale_translation_filters() {
  $filters = array();

  // Get all languages, except English
  backdrop_static_reset('language_list');
  $language_options = language_list(TRUE, TRUE);
  if (!locale_translate_english() && isset($language_options['en'])) {
    unset($language_options['en']);
  }

  $filters['string'] = array(
    'title' => t('String contains'),
    'description' => t('Leave blank to show all strings.'),
  );

  $filters['language'] = array(
    'title' => t('Language'),
    'options' => array_merge(array('all' => t('All languages'), LANGUAGE_SYSTEM => t('System (English)')), $language_options),
  );

  $filters['translation'] = array(
    'title' => t('Search in'),
    'options' => array('all' => t('Both translated and untranslated strings'), 'translated' => t('Only translated strings'), 'untranslated' => t('Only untranslated strings')),
  );

  return $filters;
}

/**
 * Return form for locale translation filters.
 *
 * @ingroup forms
 */
function locale_translation_filter_form() {
  $filters = locale_translation_filters();

  $form['filters'] = array(
    '#type' => 'fieldset',
    '#title' => t('Filter translatable strings'),
    '#collapsible' => TRUE,
    '#collapsed' => FALSE,
  );
  foreach ($filters as $key => $filter) {
    // Special case for 'string' filter.
    if ($key == 'string') {
      $form['filters']['status']['string'] = array(
        '#type' => 'search',
        '#title' => $filter['title'],
        '#description' => $filter['description'],
      );
    }
    else {
      $form['filters']['status'][$key] = array(
        '#title' => $filter['title'],
        '#type' => 'select',
        '#empty_value' => 'all',
        '#empty_option' => $filter['options']['all'],
        '#size' => 0,
        '#options' => $filter['options'],
      );
    }
    if (!empty($_SESSION['locale_translation_filter'][$key])) {
      $form['filters']['status'][$key]['#default_value'] = $_SESSION['locale_translation_filter'][$key];
    }
  }

  $form['#attributes']['class'] = array('locale-translation-filter-form');

  $form['filters']['actions'] = array(
    '#type' => 'actions',
    '#attributes' => array('class' => array('container-inline')),
  );
  $form['filters']['actions']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Filter'),
  );
  if (!empty($_SESSION['locale_translation_filter'])) {
    $form['filters']['actions']['reset'] = array(
      '#type' => 'submit',
      '#value' => t('Reset')
    );
  }

  return $form;
}

/**
 * Validate result from locale translation filter form.
 */
function locale_translation_filter_form_validate($form, &$form_state) {
  if ($form_state['values']['op'] == t('Filter') && empty($form_state['values']['language'])) {
    form_set_error('type', t('You must select something to filter by.'));
  }
}

/**
 * Process result from locale translation filter form.
 */
function locale_translation_filter_form_submit($form, &$form_state) {
  $op = $form_state['values']['op'];
  $filters = locale_translation_filters();
  switch ($op) {
    case t('Filter'):
      foreach ($filters as $name => $filter) {
        if (isset($form_state['values'][$name])) {
          $_SESSION['locale_translation_filter'][$name] = $form_state['values'][$name];
        }
      }
      break;
    case t('Reset'):
      $_SESSION['locale_translation_filter'] = array();
      break;
  }

  $form_state['redirect'] = 'admin/config/regional/translate/translate';
}


/**
 * User interface for string editing.
 *
 * @ingroup forms
 */
function locale_translate_edit_form($form, &$form_state, $lid) {
  // Fetch source string, if possible.
  $source = db_query('SELECT source, context, location FROM {locales_source} WHERE lid = :lid', array(':lid' => $lid))->fetchObject();
  if (!$source) {
    backdrop_set_message(t('String not found.'), 'error');
    backdrop_goto('admin/config/regional/translate/translate');
  }

  // Add original text to the top and some values for form altering.
  $form['original'] = array(
    '#type'  => 'item',
    '#title' => t('Original text'),
    '#markup' => check_plain(wordwrap($source->source, 0)),
  );
  if (!empty($source->context)) {
    $form['context'] = array(
      '#type' => 'item',
      '#title' => t('Context'),
      '#markup' => check_plain($source->context),
    );
  }
  $form['lid'] = array(
    '#type'  => 'value',
    '#value' => $lid
  );
  $form['location'] = array(
    '#type'  => 'value',
    '#value' => $source->location
  );

  // Include both translated and not yet translated target languages in the
  // list. The source language is English for built-in strings and the default
  // language for other strings.
  $languages = language_list();
  if (!locale_translate_english()) {
    unset($languages['en']);
  }
  $form['translations'] = array('#tree' => TRUE);
  // Approximate the number of rows to use in the default textarea.
  $rows = min(ceil(str_word_count($source->source) / 12), 10);
  foreach ($languages as $langcode => $language) {
    $form['translations'][$langcode] = array(
      '#type' => 'textarea',
      '#title' => $language->name,
      '#rows' => $rows,
      '#default_value' => '',
    );
  }

  // Fetch translations and fill in default values in the form.
  $result = db_query("SELECT DISTINCT translation, language FROM {locales_target} WHERE lid = :lid", array(':lid' => $lid));
  foreach ($result as $translation) {
    $form['translations'][$translation->language]['#default_value'] = $translation->translation;
  }

  $form['actions'] = array('#type' => 'actions');
  $form['actions']['submit'] = array('#type' => 'submit', '#value' => t('Save translations'));
  return $form;
}

/**
 * Validate string editing form submissions.
 */
function locale_translate_edit_form_validate($form, &$form_state) {
  require_once BACKDROP_ROOT . '/core/includes/locale.inc';

  foreach ($form_state['values']['translations'] as $key => $value) {
    if (!locale_string_is_safe($value)) {
      form_set_error('translations', t('The submitted string contains disallowed HTML: %string', array('%string' => $value)));
      watchdog('locale', 'Attempted submission of a translation string with disallowed HTML: %string', array('%string' => $value), WATCHDOG_WARNING);
    }
  }
}

/**
 * Process string editing form submissions.
 *
 * Saves all translations of one string submitted from a form.
 */
function locale_translate_edit_form_submit($form, &$form_state) {
  require_once BACKDROP_ROOT . '/core/includes/locale.inc';

  $lid = $form_state['values']['lid'];
  foreach ($form_state['values']['translations'] as $key => $value) {
    $translation = db_query("SELECT translation FROM {locales_target} WHERE lid = :lid AND language = :language", array(':lid' => $lid, ':language' => $key))->fetchField();
    if (!empty($value)) {
      // Only update or insert if we have a value to use.
      if (is_string($translation)) {
        db_update('locales_target')
          ->fields(array(
            'translation' => $value,
          ))
          ->condition('lid', $lid)
          ->condition('language', $key)
          ->execute();
      }
      else {
        db_insert('locales_target')
          ->fields(array(
            'lid' => $lid,
            'translation' => $value,
            'language' => $key,
          ))
          ->execute();
      }
    }
    elseif (!empty($translation)) {
      // Empty translation entered: remove existing entry from database.
      db_delete('locales_target')
        ->condition('lid', $lid)
        ->condition('language', $key)
        ->execute();
    }

    // Force JavaScript translation file recreation for this language.
    _locale_invalidate_js($key);
  }

  backdrop_set_message(t('The string has been saved.'));

  // Clear locale cache.
  _locale_invalidate_js();
  cache()->deletePrefix('locale:');

  $form_state['redirect'] = 'admin/config/regional/translate/translate';
  return;
}

/**
 * String deletion confirmation page.
 */
function locale_translate_delete_page($lid) {
  if ($source = db_query('SELECT lid, source FROM {locales_source} WHERE lid = :lid', array(':lid' => $lid))->fetchObject()) {
    return backdrop_get_form('locale_translate_delete_form', $source);
  }
  else {
    return MENU_NOT_FOUND;
  }
}

/**
 * User interface for the string deletion confirmation screen.
 *
 * @ingroup forms
 */
function locale_translate_delete_form($form, &$form_state, $source) {
  $form['lid'] = array('#type' => 'value', '#value' => $source->lid);
  return confirm_form($form, t('Are you sure you want to delete the string "%source"?', array('%source' => $source->source)), 'admin/config/regional/translate/translate', t('Deleting the string will remove all translations of this string in all languages. This action cannot be undone.'), t('Delete'), t('Cancel'));
}

/**
 * Process string deletion submissions.
 */
function locale_translate_delete_form_submit($form, &$form_state) {
  require_once BACKDROP_ROOT . '/core/includes/locale.inc';

  db_delete('locales_source')
    ->condition('lid', $form_state['values']['lid'])
    ->execute();
  db_delete('locales_target')
    ->condition('lid', $form_state['values']['lid'])
    ->execute();
  // Force JavaScript translation file recreation for all languages.
  _locale_invalidate_js();
  cache()->deletePrefix('locale:');
  backdrop_set_message(t('The string has been removed.'));
  $form_state['redirect'] = 'admin/config/regional/translate/translate';
}
