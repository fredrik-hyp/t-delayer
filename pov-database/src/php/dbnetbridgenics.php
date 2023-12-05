<?php
		// -- Copyright Vericom AB 2015	
		// -- All rights reserved

		// --- @provides PostgreSQL netbridgenics functions
		// --- @author Fredrik Nilsson <fredrik.nilsson@vericom.se>
		// --- @last-modified 2015-06-16

	
	require_once("dblib.php");
	$dbonn = DBConnect();
	
	function DBFetchNetbridgenics()
	{
		global $DB;
		$dh = $DB->prepare("SELECT * FROM netbridgenics ORDER BY name");
		$result = $dh->execute(array());
		if($result === false)
		{
			return array();
		}
		
		$result = $dh->fetchAll();
		if($result === false)
		{
			return array();
		}
		foreach($result as $idx => $bond)
		{
			$tmpnics = str_replace("{", "", $result[$idx]['netnics']);
			$tmpnics = str_replace("}", "", $tmpnics);
			$netnics = explode(",", $tmpnics);
			$result[$idx]['netnics'] = $netnics;
		}
		return $result;
	}

	function DBFetchNetbridgenicById($id)
	{
		global $DB;
		$dh = $DB->prepare("SELECT * FROM netbridgenics WHERE id=:id");
		$result = $dh->execute(array(":id" => $id));
		if($result === false)
		{
			return array();
		}
		
		return $dh->fetch();
	}

	function DBUpdateNetbridgenic($id, $name, $inuse, $description, $dhcp, $address, $netmask, $gateway, $blocktcp, $netnics)	
	{
		$netnicsposgres = '{' . implode(",", $netnics) . '}';

		global $DB;
		$dh = $DB->prepare("UPDATE netbridgenics SET name=:name, inuse=:inuse, description=:description, dhcp=:dhcp, address=:address, netmask=:netmask, gateway=:gateway, blocktcp=:blocktcp, netnics=:netnicsposgres WHERE id=:id");
		$result = $dh->execute(array(":id" => $id, 
						":name" => $name, 
						":inuse" => $inuse, 
						":description" => $description, 
						":dhcp" => $dhcp, 
						":address" => $address,
						":netmask" => $netmask,
						":gateway" => $gateway, 
						":blocktcp" => $blocktcp,
						":netnicsposgres" => $netnicsposgres
						 ));
		if($result)
		{
			return 1;				 	
		}
		else
		{
			return 0;
		}
	}
	/*
	function DBUpdateNetbridgenic($id, $netnic1id, $netnic2id, $netnic3id, $netnic4id, $netnic5id, $netnic6id, $netnic7id, $netnic8id, $name, $inuse, $description, $dhcp, $address, $netmask, $blocktcp)	
	{
		global $DB;
		$dh = $DB->prepare("UPDATE netbridgenics SET netnic1_id=:netnic1id, netnic2_id=:netnic2id, netnic3_id=:netnic3id, netnic4_id=:netnic4id, netnic5_id=:netnic5id, netnic6_id=:netnic6id, netnic7_id=:netnic7id, netnic8_id=:netnic8id, name=:name, inuse=:inuse, description=:description, dhcp=:dhcp, address=:address, netmask=:netmask, blocktcp=:blocktcp WHERE id=:id");
		$result = $dh->execute(array(":id" => $id,
									":netnic1id" => $netnic1id, 
									":netnic2id" => $netnic2id, 
									":netnic3id" => $netnic3id, 
									":netnic4id" => $netnic4id, 
									":netnic5id" => $netnic5id, 
									":netnic6id" => $netnic6id, 
									":netnic7id" => $netnic7id, 
									":netnic8id" => $netnic8id, 
									":name" => $name, 
									":inuse" => $inuse, 
									":description" => $description, 
									":dhcp" => $dhcp, 
									":address" => $address,
									":netmask" => $netmask, 
									":blocktcp" => $blocktcp
								 ));
		if($result)
		{
			return 1;				 	
		}
		else
		{
			return 0;
		}
	}
*/
	function DBAddNetbridgenic($netnic1id, $netnic2id, $netnic3id, $netnic4id, $netnic5id, $netnic6id, $netnic7id, $netnic8id, $inuse, $description)
	{
		global $DB;
		$dh = $DB->prepare("INSERT INTO netbridgenics (netnic1_id, netnic2_id, netnic3_id, netnic4_id, netnic5_id, netnic6_id, netnic7_id, netnic8_id, inuse, description) VALUES (:netnic1id,:netnic2id,:netnic3id,:netnic4id,:netnic5id,:netnic6id,:netnic7id,:netnic8id,:inuse,:description)");
		$result = $dh->execute(array(	":netnic1id" => $netnic1id,
										":netnic2id" => $netnic2id,
										":netnic3id" => $netnic3id,
										":netnic4id" => $netnic4id,
										":netnic5id" => $netnic5id,
										":netnic6id" => $netnic6id,
										":netnic7id" => $netnic7id,
										":netnic8id" => $netnic8id,
										":inuse" => $inuse,
										":description" => $description
										));
		if($result)
		{
			return $DB->lastInsertId("netbridgenics_id_seq");							
		}
		
		return 0;
	}

	function DBSetNetbridgenicarray($netnics, $id)
	{
		global $DB;
		
		$netnicsposgres = '{' . implode(",", $netnics) . '}';

		$dh = $DB->prepare("UPDATE netbridgenics set netnics=:netnicsposgres where id=:id");

		$result = $dh->execute(array(":id" => $id, ":netnicsposgres" => $netnicsposgres));

		if($result)
		{
			return 1;							
		}
		
		return 0;
	}

	
	function DBDeleteNetbridgenicById($id)
	{
		global $DB;
		$dh = $DB->prepare("DELETE FROM netbridgenics WHERE id=:id");
		$result = $dh->execute(array(":id" => $id));
		
		if($result)
		{
			return 1;
		}
		else
		{
			return 0;
		}
	}
	
	
	
?>
