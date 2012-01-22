CREATE DATABASE `teeworlds_wDM`;

USE `teeworlds_wDM`;

CREATE TABLE `Players` (
  `Id` smallint unsigned NOT NULL AUTO_INCREMENT,
  `Ip` varchar(42) NOT NULL,
  `Pseudo` varchar(16) DEFAULT NULL,
  `Pseudo_Hash` BIGINT unsigned DEFAULT NULL COMMENT 'Old System',
  `Clan` varchar(12) DEFAULT NULL,
  `Clan_Hash` BIGINT unsigned DEFAULT NULL COMMENT 'Old System',
  `Country` smallint NOT NULL,
  `Name` varchar(16) DEFAULT NULL,
  `Password` BIGINT unsigned DEFAULT NULL,
  `Last_Connect` datetime NOT NULL,
  PRIMARY KEY (`Id`),
  UNIQUE KEY `Index_Name` (`Name`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

CREATE TABLE `Stats` (
  `Id` smallint unsigned NOT NULL AUTO_INCREMENT,
  `Id_Player` smallint unsigned NOT NULL,
  `Killed` mediumint unsigned NOT NULL,
  `Dead` mediumint unsigned NOT NULL,
  `Suicide` mediumint unsigned NOT NULL,
  `Log_In` mediumint unsigned NOT NULL,
  `Fire` mediumint unsigned NOT NULL,
  `Pickup_Weapon` mediumint unsigned NOT NULL,
  `Pickup_Ninja` mediumint unsigned NOT NULL,
  `Change_Weapon` mediumint unsigned NOT NULL,
  `Time_Play` time NOT NULL,
  `Message` mediumint unsigned NOT NULL,
  `Killing_Spree` mediumint unsigned NOT NULL,
  `Max_Killing_Spree` mediumint unsigned NOT NULL,
  `Flag_Capture` mediumint unsigned NOT NULL,
  `Bonus_XP` mediumint unsigned NOT NULL,
  `Upgrade_Weapon` tinyint unsigned NOT NULL,
  `Upgrade_Life` tinyint unsigned NOT NULL,
  `Upgrade_Move` tinyint unsigned NOT NULL,
  `Upgrade_Hook` tinyint unsigned NOT NULL,
  PRIMARY KEY (`Id`),
  CONSTRAINT `Fk_Id_Player_Stats` FOREIGN KEY (`Id_Player`) REFERENCES Players(`Id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

CREATE TABLE `Confs` (
  `Id` smallint unsigned NOT NULL AUTO_INCREMENT,
  `Id_Player` smallint unsigned NOT NULL,
  `Info_Heal_Killer` BOOLEAN NOT NULL,
  `Info_XP` BOOLEAN NOT NULL,
  `Info_Level_Up` BOOLEAN NOT NULL,
  `Info_Killing_Spree` BOOLEAN NOT NULL,
  `Info_Race` BOOLEAN NOT NULL,
  `Info_Ammo` BOOLEAN NOT NULL,
  `Show_Voter` BOOLEAN NOT NULL,
  `Ammo_Absolute` BOOLEAN NOT NULL,
  `Life_Absolute` BOOLEAN NOT NULL,
  `Lock` BOOLEAN NOT NULL,
  `Race_Hammer` tinyint NOT NULL,
  `Race_Gun` tinyint NOT NULL,
  `Race_Shotgun` tinyint NOT NULL,
  `Race_Grenade` tinyint NOT NULL,
  `Race_Rifle` tinyint NOT NULL,
  PRIMARY KEY (`Id`),
  CONSTRAINT `Fk_Id_Player_Confs` FOREIGN KEY (`Id_Player`) REFERENCES Players(`Id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

