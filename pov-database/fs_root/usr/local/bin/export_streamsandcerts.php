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

	$result = array();
	$organisations = DBFetchOrganisations();

	foreach($organisations as $idx => $org)
	{
		$rootcertificate = DBFetchRootCertificateByOrganisationId($org['id']);
		$rootcertificate['clientcertificates'] = DBFetchClientCertificatesByRootCertificateId($rootcertificate['id']);
		$organisations[$idx]['rootcertificate'] = $rootcertificate;
	}

	$result['organisations'] = $organisations;

	$streaminterfaces  = DBFetchAllStreaminterfaces();
	foreach($streaminterfaces as $idx => $streaminterface)
	{
		$config = DBFetchStreaminterfaceConfigByStreaminterfaceId($streaminterface['id']);
		$source = DBFetchStreaminterfaceSourceByStreaminterfaceConfigId($config['id']);
		$sink = DBFetchStreaminterfaceSinkByStreaminterfaceConfigId($config['id']);
		$backupsources  = DBFetchStreaminterfaceBackupsourcesByStreaminterfaceSourceId($source['id']);
		$backupsinks = DBFetchStreaminterfaceBackupSinksByStreaminterfacesinkId($sink['id']);

		$config['source'] = $source;
		$config['sink'] = $sink;
		$config['backupsources'] = $backupsources;
		$config['backupsinks'] = $backupsinks;
		$streaminterfaces[$idx]['config'] = $config;
	}

	$result['streaminterfaces'] = $streaminterfaces;
	print  json_encode($result);
?>
