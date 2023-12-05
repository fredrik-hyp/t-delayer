<?php
		// -- Copyright Vericom AB 2015	
		// -- All rights reserved

		// --- @provides PostgreSQL netnics functions
		// --- @author Fredrik Nilsson <fredrik.nilsson@vericom.se>
		// --- @last-modified 2015-06-16

	
	require_once("dblib.php");
	$dbonn = DBConnect();
	
	function DBFetchNetnics()
	{
		global $DB;
		$dh = $DB->prepare("SELECT * FROM netnics ORDER BY physicalname");
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

	function DBFetchNetnicsInUse()
	{
		global $DB;
		$dh = $DB->prepare("SELECT * FROM netnics WHERE inuse='t' ORDER BY physicalname");
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


 	function DBFetchNetnicById($id)
	{
		global $DB;
		$dh = $DB->prepare("SELECT * FROM netnics WHERE id=:id");
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
	
	function DBFetchNetnicByPhysicalName($physicalname)
	{
		global $DB;
		$dh = $DB->prepare("SELECT * FROM netnics WHERE physicalname=:physicalname");
		$result = $dh->execute(array(":physicalname" => $physicalname));
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
	
	function DBUpdateNetnic($id, $physicalname, $name, $inuse, $description, $dhcp, $address, $netmask)
	{
		global $DB;
		$dh = $DB->prepare("UPDATE netnics SET physicalname=:physicalname, name=:name, inuse=:inuse, description=:description, dhcp=:dhcp, address=:address, netmask=:netmask WHERE id=:id");
		$result = $dh->execute(array( ":id" => $id,
										":physicalname" => $physicalname,
										":name" => $name,
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
	
	function DBAddNetnic($physicalname, $name, $inuse, $description, $dhcp, $address, $netmask)
	{
		global $DB;
		$dh = $DB->prepare("INSERT INTO netnics (physicalname, name, inuse, description, dhcp, address, netmask) VALUES (:physicalname, :name, :inuse, :description, :dhcp, :address, :netmask)");
		$result = $dh->execute(array(":physicalname" => $physicalname,
									":name" => $name,
									":inuse" => $inuse,
									":description" => $description,
									":dhcp" => $dhcp,
									":address" => $address,
									":netmask" => $netmask));
		if($result)
		{
			return $DB->lastInsertId("netnics_id_seq");
		}
		
		return 0;
	}
	
	function DBSetNetnicBlockTCP($id, $blocktcp)
	{
		global $DB;
		$dh = $DB->prepare("UPDATE netnics SET blocktcp=:blocktcp WHERE id=:id");
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
	
	function DBUnsetNetnicsBlockTCP()
	{
		global $DB;
		$dh = $DB->prepare("UPDATE netnics SET blocktcp='f'");
		$result = $dh->execute(array());
		
		if($result === false)
		{
			return false;
		}
		
		return true;
	}
	
	function DBDeleteNetnicById($id)
	{
		global $DB;
		$dh = $DB->prepare("DELETE FROM netnics WHERE id=:id");
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
