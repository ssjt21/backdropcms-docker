<?php
/**
 * @file
 * Plugin to provide access control based upon role membership.
 */
class UserRoleLayoutAccess extends LayoutAccessNegatable {
  /**
   * Constructor for a Layout access rule.
   */
  function __construct($plugin_name, array $data = array()) {
    parent::__construct($plugin_name, $data);
    $this->settings += array(
      'roles' => array(),
    );
  }

  /**
   * {@inheritdoc}
   */
  function summary() {
    $role_names = backdrop_map_assoc($this->settings['roles']);
    $roles = array_intersect_key(user_roles(), $role_names);
    if (empty($roles)) {
      return t('User can have any role');
    }
    if ($this->settings['negate']) {
      return format_plural(count($roles), 'User does not have the role @role', 'User does not have one of the following roles: @role', array('@role' => implode(', ', $roles)));
    }
    else {
      return format_plural(count($roles), 'User has the role @role', 'User has one of the following roles: @role', array('@role' => implode(', ', $roles)));
    }
  }

  /**
   * {@inheritdoc}
   */
  function checkAccess() {
    $account = $this->contexts['user']->data;
    $roles = $account->roles;
    $role_matches = count(array_intersect($this->settings['roles'], $roles)) > 0;
    if ($this->settings['negate']) {
      return !$role_matches;
    }
    else {
      return $role_matches;
    }
  }

  /**
   * {@inheritdoc}
   */
  function form(&$form, &$form_state) {
    parent::form($form, $form_state);
    $form['roles'] = array(
      '#type' => 'checkboxes',
      '#title' => t('Role'),
      '#default_value' => $this->settings['roles'],
      '#options' => user_roles(),
      '#description' => t('Only the checked roles will be granted access (or denied access, if "Reverse" is checked).'),
    );
  }

  /**
   * {@inheritdoc}
   */
  function formSubmit($form, &$form_state) {
    parent::formSubmit($form, $form_state);
    $this->settings['roles'] = array_keys(array_filter($form_state['values']['roles']));
  }
}
