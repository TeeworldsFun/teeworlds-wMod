USE teeworlds_wDM;
LOAD DATA LOCAL INFILE 'Statistiques.players' INTO TABLE Players FIELDS TERMINATED BY ';' LINES TERMINATED BY '\n' (Id, Ip, Pseudo_Hash, Clan_Hash, Country, Last_Connect);
LOAD DATA LOCAL INFILE 'Statistiques.stats' INTO TABLE Stats FIELDS TERMINATED BY ';' LINES TERMINATED BY '\n' (Id_Player, Killed, Dead, Suicide, Log_In, Fire, Pickup_Weapon, Pickup_Ninja, Change_Weapon, Time_Play, Message, Killing_Spree, Max_Killing_Spree,  Flag_Capture, Bonus_XP, Upgrade_Weapon, Upgrade_Life,  Upgrade_Move, Upgrade_Hook);
LOAD DATA LOCAL INFILE 'Statistiques.confs' INTO TABLE Confs FIELDS TERMINATED BY ';' LINES TERMINATED BY '\n' (Id_Player, Info_Heal_Killer, Info_XP , Info_Level_Up, Info_Killing_Spree, Info_Race, Info_Ammo, Show_Voter, Ammo_Absolute, Life_Absolute, `Lock`, Race_Hammer, Race_Gun, Race_Shotgun, Race_Grenade, Race_Rifle);

