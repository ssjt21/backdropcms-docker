<?php
/**
 * @file
 * Action callbacks for User module.
 */

/**
 * Blocks a specific user account.
 *
 * @param User $account
 *   The user entity whose account will be blocked.
 *
 * @ingroup actions
 */
function user_block_user_action(User $account, &$context) {
  global $user;

  // Don't block User 1, or the user performing the action.
  if ($account->uid === '1') {
    backdrop_set_message(t('The user account %admin_name (user with ID 1) cannot be blocked.', array('%admin_name' => $account->name)), 'warning');
    return FALSE;
  }
  elseif ($account->uid == $user->uid) {
    backdrop_set_message(t('You cannot block your own user account.'), 'warning');
    return FALSE;
  }

  if ($account->status == 1) {
    $account->status = 0;
    $account->save();
  }
}

/**
 * Unblocks a specific user account.
 *
 * @param User $account
 *   The user entity whose account will be unblocked.
 *
 * @ingroup actions
 */
function user_unblock_user_action($account, &$context) {
  if ($account->status == 0) {
    $account->status = 1;
    $account->save();
  }
}

/**
 * Queues a specific user account for cancellation.
 *
 * @param User $account
 *   The user entity whose account will be cancelled.
 *
 * @ingroup actions
 */
function user_cancel_user_action($account, &$context) {
  // Save the list of user accounts to be deleted in the session. Append to 
  // the existing list if within the last minute, otherwise start a new list
  // of user accounts.
  $last_action_time = 0;
  if (isset($_SESSION['user_cancel_action'])) {
    $last_action_time = $_SESSION['user_cancel_action']['timestamp'];
  }
  if (REQUEST_TIME - $last_action_time > 60) {
    $_SESSION['user_cancel_action'] = array(
      'timestamp' => REQUEST_TIME,
      'uids' => array(),
    );
  }

  $_SESSION['user_cancel_action']['uids'][] = $account->uid;

  $context['redirect'] = 'admin/people/cancel';
}

/**
 * Adds a role to a user account.
 *
 * @param User $account
 *   The user entity to which the role will be added.
 *
 * @ingroup actions
 */
function user_add_role_action($account, &$context) {
  $role_name = $context['action_info']['role_name'];
  if (!in_array($role_name, $account->roles)) {
    $account->roles[] = $role_name;
    $account->save();
  }
}

/**
 * Removes a role from a user account.
 *
 * @param User $account
 *   The user entity from which the role will be removed.
 *
 * @ingroup actions
 */
function user_remove_role_action($account, &$context) {
  $role_name = $context['action_info']['role_name'];
  $index = array_search($role_name, $account->roles);
  if ($index !== FALSE) {
    unset($account->roles[$index]);
    $account->save();
  }
}
