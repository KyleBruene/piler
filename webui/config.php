<?php

define('LANG', 'en');
define('THEME', 'default');

define('SITE_NAME', 'demo.mailpiler.org');
define('SITE_URL', 'http://demo.mailpiler.org/');

define('ENABLE_AUDIT', 1);
define('MEMCACHED_ENABLED', 0);
define('PASSWORD_CHANGE_ENABLED', 0);
define('ENABLE_STATISTICS', 1);
define('ENABLE_HISTORY', 1);
define('ENABLE_REMOTE_IMAGES', '0');
define('ENABLE_ON_THE_FLY_VERIFICATION', 1);
define('ENABLE_LDAP_IMPORT_FEATURE', 0);

define('HOLD_EMAIL', 0);

define('REMOTE_IMAGE_REPLACEMENT', '/view/theme/default/images/remote.gif');
define('ICON_ARROW_UP', '/view/theme/default/images/arrowup.gif');
define('ICON_ARROW_DOWN', '/view/theme/default/images/arrowdown.gif');
define('ICON_ATTACHMENT', '/view/theme/default/images/attachment_icon.png');
define('ICON_TAG', '/view/theme/default/images/tag_blue.png');
define('ICON_GREEN_OK', '/view/theme/default/images/green_ok.png');
define('ICON_RED_X', '/view/theme/default/images/red_x.png');

define('MAX_CGI_FROM_SUBJ_LEN', 45);
define('PAGE_LEN', 20);
define('MAX_NUMBER_OF_FROM_ITEMS', 5);
define('MAX_SEARCH_HITS', 1000);

define('LOCALHOST', '127.0.0.1');
define('PILER_HOST', '1.2.3.4');
define('PILER_PORT', 25);
define('SMARTHOST', '127.0.0.1');
define('SMARTHOST_PORT', 10026);
define('SMTP_DOMAIN', 'mailpiler.org');
define('SMTP_FROMADDR', 'no-reply@mailpiler.org');

define('EOL', "\n");

define('PILER_HEADER_FIELD', 'X-piler-id: ');

define('DEFAULT_POLICY', 'default_policy');

define('DIR_BASE', '/var/www/demo.mailpiler.org/');
define('DIR_SYSTEM', DIR_BASE . 'system/');
define('DIR_MODEL', DIR_BASE . 'model/');
define('DIR_DATABASE', DIR_BASE . 'system/database/');
define('DIR_IMAGE', DIR_BASE . 'image/');
define('DIR_LANGUAGE', DIR_BASE . 'language/');
define('DIR_APPLICATION', DIR_BASE . 'controller/');
define('DIR_THEME', DIR_BASE . 'view/theme/');
define('DIR_REPORT', DIR_BASE . 'reports/');
define('DIR_LOG', DIR_BASE . 'log/');

define('DIR_STORE', '/var/piler/store');
define('DIR_STAT', '/var/piler/stat');

define('DECRYPT_BINARY', '/usr/local/bin/pilerget');
define('DECRYPT_BUFFER_LENGTH', 65536);

define('QSHAPE_ACTIVE_INCOMING', DIR_STAT . '/active+incoming');
define('QSHAPE_ACTIVE_INCOMING_SENDER', DIR_STAT . '/active+incoming-sender');
define('QSHAPE_DEFERRED', DIR_STAT . '/deferred');
define('QSHAPE_DEFERRED_SENDER', DIR_STAT . '/deferred-sender');

define('CPUSTAT', DIR_STAT . '/cpu.stat');
define('AD_SYNC_STAT', DIR_STAT . '/adsync.stat');
define('ARCHIVE_SIZE', DIR_STAT . '/archive.size');
define('LOCK_FILE', DIR_LOG . 'lock');

define('DB_DRIVER', 'mysql');
define('DB_PREFIX', '');
define('DB_HOSTNAME', 'localhost');
define('DB_USERNAME', 'piler');
define('DB_PASSWORD', 'q57tPYem');
define('DB_DATABASE', 'piler');

define('TABLE_USER', 'user');
define('TABLE_EMAIL', 'email');
define('TABLE_META', 'metadata');
define('TABLE_ATTACHMENT', 'attachment');
define('TABLE_SEARCH', 'search');
define('TABLE_EMAIL_LIST', 'email_groups');
define('TABLE_TAG', 'tag');
define('TABLE_USER_SETTINGS', 'user_settings');
define('TABLE_REMOTE', 'remote');
define('TABLE_DOMAIN', 'domain');
define('TABLE_COUNTER', 'counter');
define('TABLE_AUDIT', 'audit');
define('TABLE_ARCHIVING_RULE', 'archiving_rule');
define('VIEW_MESSAGES', 'messages');

define('SPHINX_DRIVER', 'sphinx');
define('SPHINX_DATABASE', 'sphinx');
define('SPHINX_HOSTNAME', '127.0.0.1:9306');
define('SPHINX_MAIN_INDEX', 'main1');
define('SPHINX_TAG_INDEX', 'tag1');


define('LDAP_IMPORT_CONFIG_FILE', '/usr/local/etc/ldap-import.cfg');

define('DN_MAX_LEN', 255);
define('USE_EMAIL_AS_USERNAME', 1);
define('LDAP_IMPORT_MINIMUM_NUMBER_OF_USERS_TO_HEALTH_OK', 100);

define('PREVIEW_WIDTH', 1024);
define('PREVIEW_HEIGTH', 700);

define('SIZE_X', 430);
define('SIZE_Y', 250);

define('HELPURL', '');

define('AUDIT_DATE_FORMAT', 'Y.m.d H:i');
define('SEARCH_HIT_DATE_FORMAT', 'Y.m.d');

define('DATE_FORMAT', '(Y.m.d.)');
define('TIMEZONE', 'Europe/Budapest');

define('HISTORY_REFRESH', 60);

define('FROM_LENGTH_TO_SHOW', 28);

define('SEARCH_HELPER_URL', SITE_URL . 'search-helper.php');
define('AUDIT_HELPER_URL', SITE_URL . 'audit-helper.php');

define('SAVE_SEARCH_URL', SITE_URL . 'index.php?route=search/save');

define('HEALTH_WORKER_URL', SITE_URL . 'index.php?route=health/worker');
define('HEALTH_REFRESH', 60);
define('HEALTH_RATIO', 80);

define('LOG_FILE', DIR_LOG . 'webui.log');
define('LOG_DATE_FORMAT', 'd-M-Y H:i:s');

define('MAX_AUDIT_HITS', 1000);

define('MIN_PASSWORD_LENGTH', 6);

define('CGI_INPUT_FIELD_WIDTH', 50);
define('CGI_INPUT_FIELD_HEIGHT', 7);

define('MEMCACHED_PREFIX', '_piler_webui:');
define('MEMCACHED_TTL', 3600);

$memcached_servers = array(
      array('127.0.0.1', 11211)
                          );

$counters = array('_piler:rcvd', '_piler:virus', '_piler:duplicate', '_piler:ignore', '_piler:counters_last_update');

$health_smtp_servers = array( array(PILER_HOST, PILER_PORT, "piler"), array(SMARTHOST, SMARTHOST_PORT, "smarthost") );

$partitions_to_monitor = array('/', '/home', '/var', '/tmp');


$langs = array(
                'hu',
                'en'
               );


$themes = array(
                'default'
               );


define('ACTION_ALL', 0);
define('ACTION_UNKNOWN', 1);
define('ACTION_LOGIN', 2);
define('ACTION_LOGIN_FAILED', 3);
define('ACTION_LOGOUT', 4);
define('ACTION_VIEW_MESSAGE', 5);
define('ACTION_VIEW_HEADER', 6);
define('ACTION_UNAUTHORIZED_VIEW_MESSAGE', 7);
define('ACTION_RESTORE_MESSAGE', 8);
define('ACTION_DOWNLOAD_MESSAGE', 9);
define('ACTION_SEARCH', 10);
define('ACTION_SAVE_SEARCH', 11);
define('ACTION_CHANGE_USER_SETTINGS', 12);

define('ACTION_REMOVE_MESSAGE', 13);
define('ACTION_UNAUTHORIZED_REMOVE_MESSAGE', 14);


define('NOW', time());

define('SIMPLE_SEARCH', 0);
define('ADVANCED_SEARCH', 1);
define('EXPERT_SEARCH', 2);

?>