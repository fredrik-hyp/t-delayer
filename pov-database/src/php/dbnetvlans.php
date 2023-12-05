<?php
		// -- Copyright Vericom AB 2015	
		// -- All rights reserved

		// --- @provides PostgreSQL netnics functions
		// --- @author Fredrik Nilsson <fredrik.nilsson@vericom.se>
		// --- @last-modified 2015-06-16

	
	require_once("dblib.php");
	$dbonn = DBConnect();
	
	function DBFetchNetvlans()
	{
		global $DB;
		$dh = $DB->prepare("SELECT * FROM netvlans ORDER BY netnicid, netbridgenic_id, vlanid");
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
		
		return $result;
	}
	
	function DBFetchNetvlanById($id)
	{
		global $DB;
		$dh = $DB->prepare("SELECT * FROM netvlans WHERE id=:id");
		$result = $dh->execute(array(":id" => $id));
		if($result === false)
		{
			return array();
		}
		
		$result = $dh->fetch();
		if($result === false)
		{
			return array();
		}
		
		return $result;
	}

	function DBFetchNetvlansByNetnicId($netnicid)
	{
		global $DB;
		$dh = $DB->prepare("SELECT * FROM netvlans WHERE netnic_id=:netnicid");
		$result = $dh->execute(array(":netnicid" => $netnicid));
		if($result === false)
		{
			return array();
		}
		
		$result = $dh->fetchAll();
		if($result === false)
		{
			return array();
		}
		
		return $result;
	}
	
	function DBFetchNetvlansByNetbridgenicId($netbridgenicid)
	{
		global $DB;
		$dh = $DB->prepare("SELECT * FROM netvlans WHERE netbridgenic_id=:netbridgenicid");
		$result = $dh->execute(array(":netbrdigenicid" => $netbridgenicid));
		if($result === false)
		{
			return array();
		}
		
		$result = $dh->fetchAll();
		if($result === false)
		{
			return array();
		}
		
		return $result;
	}
	function DBFetchNetvlanByNetnicIdAndVlanid($netnicid, $vlanid)
	{
		global $DB;
		$dh = $DB->prepare("SELECT * FROM netvlans WHERE netnic_id=:netnicid AND vlanid=:vlanid ORDER BY vlanid");
		$result = $dh->execute(array(":vlanid" => $vlanid,
									 ":netnicid" => $netnicid));
		if($result === false)
		{
			return array();
		}
		
		$result = $dh->fetch();
		if($result === false)
		{
			return array();
		}
		
		return $result;
	}
	
	function DBUpdateNetvlan($id, $netnicid, $netbridgenicid, $vlanid, $inuse, $description, $dhcp, $address, $netmask)
	{
		global $DB;
		if($dhcp === true || $dhcp == 't')
		{
			$dhcp = 't';
		}
		else
		{
			$dhcp = 'f';
		}
		$dh = $DB->prepare("UPDATE netvlans SET netnic_id=:netnicid, netbridgenic_id=:netbridgenicid, vlanid=:vlanid, inuse=:inuse, description=:description, dhcp=:dhcp, address=:address, netmask=:netmask WHERE id=:id");
		$result = $dh->execute(array(":id" => $id,
									 ":netnicid" => $netnicid,
								 	 ":netbridgenicid" => $netbridgenicid,
								 	 ":vlanid" => $vlanid,
								 	 ":inuse" => $inuse,
									 ":description" => $description,
									 ":dhcp" => $dhcp,
								 	 ":address" => $address,
									 ":netmask" => $netmask));
		if($result)
		{
			return 1;					 	
		}
		else
		{
			return 0;
		}
	}
	
	function DBAddNetvlan($netnicid, $netbridgenicid, $vlanid, $inuse, $description, $dhcp, $address, $netmask)
	{
		global $DB;
		if($dhcp === true || $dhcp == 't')
		{
			$dhcp = 't';
		}
		else
		{
			$dhcp = 'f';
		}
		$dh = $DB->prepare("INSERT INTO netvlans (netnic_id, netbridgenic_id, vlanid, inuse, description, dhcp, address, netmask) VALUES (:netnicid, :netbridgenicid, :vlanid, :inuse, :description, :dhcp, :address, :netmask) ");
		$result = $dh->execute(array(	":netnicid" => $netnicid,
										":netbridgenicid" => $netbridgenicid,
										":vlanid" => $vlanid,
										":inuse" => $inuse,
										":description" => $description,
										":dhcp" => $dhcp,
										":address" => $address,
										":netmask" => $netmask));
		if($result)
		{
			return $DB->lastInsertId("netvlans_id_seq");;
		}
		return 0;
	}
	
	function DBSetNetvlanBlockTCP($id, $blocktcp)
	{
		global $DB;
		$dh = $DB->prepare("UPDATE netvlans SET blocktcp=:blocktcp WHERE id=:id");
		$result = $dh->execute(array(":id" => $id,
									 ":blocktcp" => $blocktcp));
									 
		if($result)
		{
			return 1;					 	
		}
		else
		{
			return 0;
		}
	}
	
	function DBUnsetNetvlansBlockTCP()
	{
		global $DB;
		$dh = $DB->prepare("UPDATE netvlans SET blocktcp='f'");
		$result = $dh->execute(array());
		
		if($result === false)
		{
			return false;
		}
		
		return true;
	}
	
	function DBDeleteNetvlanById($id)
	{
		global $DB;
		$dh = $DB->prepare("DELETE FROM netvlans WHERE id=:id");
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
