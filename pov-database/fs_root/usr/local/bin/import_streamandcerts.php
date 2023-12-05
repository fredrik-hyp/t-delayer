#!/usr/bin/php
<?php
	require_once("/var/db/dblib.php");
	require_once("/var/db/dbstreaminterfaces.php");
	require_once("/var/db/dbstreaminterfaceconfigs.php");
	require_once("/var/db/dbstreaminterfacesources.php");
	require_once("/var/db/dbstreaminterfacesinks.php");
	require_once("/var/db/dbstreaminterfacebackupsources.php");
	require_once("/var/db/dbstreaminterfacebackupsinks.php");
	require_once("/var/db/dbrootcertificates.php");
	require_once("/var/db/dbclientcertificates.php");
	require_once("/var/db/dborganisations.php");

	function buildInsertStatement($tablename, $arr)
	{
		$keys = array_keys($arr);
		$columns = "";
		$values = "";
		foreach($keys as $key)
		{
			if(is_numeric($key) || is_array($arr[$key]))
			{
				continue;
			}
			$columns .=  $key . ",";
			$values  .=  ":" . $key  . ",";
		}
		$columns = rtrim($columns, ",");
		$values = rtrim($values, ",");

		return "INSERT INTO $tablename ($columns) VALUES ($values)";
	}

	function buildUpdateStatement($tablename, $arr)
	{
		$keys = array_keys($arr);
		$values = "";
		foreach($keys as $key)
		{
		 	if(is_numeric($key) || is_array($arr[$key]) || $key == "id")
			{
				continue;
			}
			$values .= " $key=:$key,";
		}

		$values = rtrim($values, ",");
		return "UPDATE $tablename SET $values WHERE id=:id";
	}

	function buildExecuteArray($arr, $removeid = false)
	{
		$keys = array_keys($arr);
		$result =  array();

		foreach($keys as $key)
		{
			if(is_numeric($key) || is_array($arr[$key]) || ($removeid && $key == "id"))
			{
				continue;
			}

			$arrkey = ":" . $key;
			
			if(is_bool($arr[$key]))
			{
				if($arr[$key])
				{
					$result[$arrkey] = 't';
				}
				else
				{
					$result[$arrkey] = 'f';					
				}
				continue;
			}
			else
			{
				$result[$arrkey] = $arr[$key];
			}
		}

		return $result;
	}

	function ClearAndResetOrgTable()
	{
		global $dbconn;
		$dbconn->beginTransaction();
		try
		{
			$sth = $dbconn->prepare("DELETE FROM streaminterfacelivestatistics");
			$sth->execute(array());
		
			$sth = $dbconn->prepare("delete from organisations");
			$sth->execute(array());
		}catch(Exception $e)
		{
			$dbconn->rollBack();
			print 'Caught exception: '.  $e->getMessage(). "\n";
			exit(50);
		}
		$dbconn->commit();
		
		$sth = $dbconn->prepare("ALTER SEQUENCE organisations_id_seq RESTART WITH 1");
		$sth->execute(array());
	}

	function ClearAndResetCertTables()
	{
		global $dbconn;

		$dbconn->beginTransaction();
		try
		{
			$sth = $dbconn->prepare("DELETE FROM streaminterfacelivestatistics");
			$sth->execute(array());
		
			$sth = $dbconn->prepare("DELETE FROM clientcertificates");
			$sth->execute(array());
		}catch(Exception $e)
		{
			$dbconn->rollBack();
			print 'Caught exception: '.  $e->getMessage(). "\n";
			exit(50);
		}
		
		$dbconn->commit();
		
		$sth = $dbconn->prepare("ALTER SEQUENCE clientcertificates_id_seq RESTART WITH 1");
		$sth->execute(array());

		$sth = $dbconn->prepare("delete from clientcertificates");
		$sth->execute(array());

		$sth = $dbconn->prepare("ALTER SEQUENCE rootcertificates_id_seq RESTART WITH 1");
		$sth->execute(array());

	}

	function ClearAndResetStreamBackupTables()
	{
		global $dbconn;
		
		$sth = $dbconn->prepare("DELETE FROM streaminterfacebackupsources");
		$result = $sth->execute(array());
		
		
		$sth = $dbconn->prepare("DELETE FROM streaminterfacebackupsinks");
		$result = $sth->execute(array());
	}

	function ImportOrgsAndCerts($contents)
	{
		global $dbconn;

		$organisations=$contents['organisations'];
		foreach($organisations as $org)
		{
			$org = (array)$org;
			$rootcert = (array)$org['rootcertificate'];
			$clientcerts = (array)$rootcert['clientcertificates'];

			unset($org['rootcertificate']);
			unset($rootcert['clientcertificates']);
			
			$sth = $dbconn->prepare(buildInsertStatement("organisations", $org));
			$result = $sth->execute(buildExecuteArray($org));
			if($result === false)
			{
				print "Could not add organisations!\n";
				exit(10);
			}

			$sth = $dbconn->prepare(buildInsertStatement("rootcertificates", $rootcert));
			$result = $sth->execute(buildExecuteArray($rootcert));
			if($result === false)
			{
				print "Could not add rootcertificate!\n";
				exit(11);
			}

			foreach($clientcerts as $clientcert)
			{
				$clientcert = (array)$clientcert;
				$sth = $dbconn->prepare(buildInsertStatement("clientcertificates", $clientcert));
				$result = $sth->execute(buildExecuteArray($clientcert));
				if($result === false)
				{
					print "Could not add clientcertificate\n";
					exit(12);
				}

			}
		}

	}

	function ImportStreamSettings($contents)
	{
		global $dbconn;
		
		$stintfs = $contents['streaminterfaces'];
		foreach($stintfs as $stintf)
		{
			$stintf = (array)$stintf;
			$config = (array)$stintf['config'];
			unset($stintf['config']);
			
			$source = (array)$config['source'];
			unset($config['source']);
			
			$sink = (array)$config['sink'];
			unset($config['sink']);
			
			$backupsources = (array)$config['backupsources'];
			unset($config['backupsources']);
			
			$backupsinks = (array)$config['backupsinks'];
			unset($config['backupsinks']);
			
			$stat = buildUpdateStatement("streaminterfaces", $stintf);
			$sth = $dbconn->prepare($stat);
			
			$execarr = buildExecuteArray($stintf);
			$result = $sth->execute($execarr);
			if($result === false)
			{
				printf("Could not update streaminterface!\n");
				exit(20);
			}

			$sth = $dbconn->prepare( buildUpdateStatement("streaminterfaceconfigs", $config));

			$execarr = buildExecuteArray($config);
			$result = $sth->execute($execarr);

			if($result === false)
			{
				printf("Could not update streaminterfaceconfig!\n");
				exit(21);
			}

			$sth = $dbconn->prepare(buildUpdateStatement("streaminterfacesources", $source));
			$result = $sth->execute(buildExecuteArray($source));

			if($result === false)
			{
				printf("Could not update streaminterfacesource!\n");
				exit(22);
			}

			$sth = $dbconn->prepare(buildUpdateStatement("streaminterfacesinks", $sink));
			$result = $sth->execute(buildExecuteArray($sink));

			if($result === false)
			{
				print("Could not update streaminterfacesink!\n");
				exit(23);
			}
			
			if(!empty($backupsources))
			{
				foreach($backupsources as $backupsource)
				{
					$backupsource = (array)$backupsource;
					$sth = $dbconn->prepare(buildInsertStatement("streaminterfacebackupsources", $backupsource));
					$result = $sth->execute(buildExecuteArray($backupsource));
					if($result === false)
					{
						printf("Could not insert into streaminterfacebackupsources!\n");
						exit(24);
					}
					
				}
			}
			
			if(!empty($backupsinks))
			{
				foreach($backupsinks as $backupsink)
				{
					$backupsink = (array)$backupsink;
					$sth = $dbconn->prepare(buildInsertStatement("streaminterfacebackupsinks", $backupsink));
					$result = $sth->execute(buildExecuteArray($backupsink));
					if($result === false)
					{
						printf("Could not insert into streaminterfacebackupsinks!\n");
						exit(24);
					}
					
				}
				
			}	
		}
	}

	if($argc == 1)
	{
	  print("No input file given!\n");
	  exit(1);
	}

	$contents = file_get_contents($argv[1]);
	if($contents === false)
	{
		print("Could not read input file: " . $argv[1] . "\n");
		exit(2);
	}

	$contents = json_decode($contents);
	if($contents === false)
	{
		print("could not decode content\n");
		exit(3);
	}

	$contents = (array)$contents;
	$dbconn = DBConnect();
	//  reset the  organisation and certificates database tables
	ClearAndResetOrgTable();
	ClearAndResetCertTables();

	ClearAndResetStreamBackupTables();
	// import organssations and certificates
	ImportOrgsAndCerts($contents);

	// import streaminterfacesettings
	ImportStreamSettings($contents);
?>
