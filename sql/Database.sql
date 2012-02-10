CREATE DATABASE `teeworlds_wDM`;

USE `teeworlds_wDM`;

CREATE TABLE `Players_Stats` (
  `Id` smallint unsigned NOT NULL AUTO_INCREMENT,
  `Ip` varchar(42) NOT NULL DEFAULT '0.0.0.0',
  `Pseudo` varchar(16) DEFAULT NULL,
  `Clan` varchar(12) DEFAULT NULL,
  `Country` smallint NOT NULL DEFAULT -1,
  `Name` varchar(16) NOT NULL,
  `Password` BIGINT unsigned NOT NULL,
  `Last_Connect` datetime NOT NULL DEFAULT '1970-01-01 01:00:00',
  `Level` mediumint unsigned NOT NULL DEFAULT 0,
  `Score` mediumint unsigned NOT NULL DEFAULT 0,
  `Killed` mediumint unsigned NOT NULL DEFAULT 0,
  `Dead` mediumint unsigned NOT NULL DEFAULT 0,
  `Suicide` mediumint unsigned NOT NULL DEFAULT 0,
  `Rapport` float unsigned NOT NULL DEFAULT 0,
  `Log_In` mediumint unsigned NOT NULL DEFAULT 0,
  `Fire` mediumint unsigned NOT NULL DEFAULT 0,
  `Pickup_Weapon` mediumint unsigned NOT NULL DEFAULT 0,
  `Pickup_Ninja` mediumint unsigned NOT NULL DEFAULT 0,
  `Change_Weapon` mediumint unsigned NOT NULL DEFAULT 0,
  `Time_Play` time NOT NULL DEFAULT 0,
  `Message` mediumint unsigned NOT NULL DEFAULT 0,
  `Killing_Spree` mediumint unsigned NOT NULL DEFAULT 0,
  `Max_Killing_Spree` mediumint unsigned NOT NULL DEFAULT 0,
  `Flag_Capture` mediumint unsigned NOT NULL DEFAULT 0,
  `Bonus_XP` mediumint unsigned NOT NULL DEFAULT 0,
  `Upgrade_Weapon` tinyint unsigned NOT NULL DEFAULT 0,
  `Upgrade_Life` tinyint unsigned NOT NULL DEFAULT 0,
  `Upgrade_Move` tinyint unsigned NOT NULL DEFAULT 0,
  `Upgrade_Hook` tinyint unsigned NOT NULL DEFAULT 0,
  `Info_Heal_Killer` BOOLEAN NOT NULL DEFAULT 1,
  `Info_XP` BOOLEAN NOT NULL DEFAULT 1,
  `Info_Level_Up` BOOLEAN NOT NULL DEFAULT 1,
  `Info_Killing_Spree` BOOLEAN NOT NULL DEFAULT 1,
  `Info_Race` BOOLEAN NOT NULL DEFAULT 1,
  `Info_Ammo` BOOLEAN NOT NULL DEFAULT 1,
  `Show_Voter` BOOLEAN NOT NULL DEFAULT 1,
  `Ammo_Absolute` BOOLEAN NOT NULL DEFAULT 1,
  `Life_Absolute` BOOLEAN NOT NULL DEFAULT 1,
  `Lock` BOOLEAN NOT NULL DEFAULT 0,
  `Race_Hammer` tinyint NOT NULL DEFAULT 0,
  `Race_Gun` tinyint NOT NULL DEFAULT 0,
  `Race_Shotgun` tinyint NOT NULL DEFAULT 0,
  `Race_Grenade` tinyint NOT NULL DEFAULT 0,
  `Race_Rifle` tinyint NOT NULL DEFAULT 0,
  PRIMARY KEY (`Id`),
  UNIQUE KEY `Name` (`Name`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

