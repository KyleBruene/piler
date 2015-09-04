-- 2015.03.10

alter table archiving_rule add column `body` varchar(128) default null;
alter table retention_rule add column `body` varchar(128) default null;

create unique index `entry` on archiving_rule (`domain`,`from`,`to`,`subject`,`body`,`_size`,`size`,`attachment_name`,`attachment_type`,`_attachment_size`,`attachment_size`,`spam`);
create unique index `entry` on retention_rule (`domain`,`from`,`to`,`subject`,`body`,`_size`,`size`,`attachment_name`,`attachment_type`,`_attachment_size`,`attachment_size`,`spam`);

-- 2015.03.29

create index metadata_idx10 on metadata(`from`);

create table if not exists `legal_hold` (
   email varchar(128) unique not null
) Engine=InnoDB;

create table if not exists `timestamp` (
  `id` bigint unsigned not null auto_increment,
  `start_id` bigint default 0,
  `stop_id` bigint default 0,
  `hash_value` char(40),
  `count` int default 0,
  `response_time` bigint default 0,
  `response_string` blob not null,
  primary key (`id`)
) Engine=InnoDB;



-- 2015.08.28

alter table archiving_rule add column `folder_id` int default 0;
alter table retention_rule add column `folder_id` int default 0;

-- 2015.09.04

create table if not exists `folder_rule` (
   `id` bigint unsigned not null auto_increment,
   `domain` varchar(128) default null,
   `from` varchar(128) default null,
   `to` varchar(128) default null,
   `subject` varchar(128) default null,
   `body` varchar(128) default null,
   `_size` char(2) default null,
   `size` int default 0,
   `attachment_name` varchar(128) default null,
   `attachment_type` varchar(128) default null,
   `_attachment_size` char(2) default null,
   `attachment_size` int default 0,
   `spam` tinyint(1) default -1,
   `days` int default 0,
   `folder_id` int default 0,
   primary key (`id`),
   unique (`domain`,`from`,`to`,`subject`,`body`,`_size`,`size`,`attachment_name`,`attachment_type`,`_attachment_size`,`attachment_size`,`spam`)
) ENGINE=InnoDB;



