<?php
		// -- Copyright Vericom AB 2015	
		// -- All rights reserved

		// --- @provides PostgreSQL netroutes functions
		// --- @author Fredrik Nilsson <fredrik.nilsson@vericom.se>
		// --- @last-modified 2015-06-16

	
	require_once("dblib.php");
	$dbonn = DBConnect();
	
	function DBFetchNetroutes()
	{
		global $DB;
		$dh = $DB->prepare("SELECT * FROM netroutes ORDER BY network");
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
	
	function DBFetchNetrouteById($id)
	{
		global $DB;
		$dh = $DB->prepare("SELECT * FROM netroutes WHERE id=:id");
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

	
	function DBFetchNetroutesByNetnicid($netnicid)
	{
		global $DB;
		$dh = $DB->prepare("SELECT * FROM netroutes WHERE netnic_id=:netnicid and netvlan_id IS NULL");
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
	
	function DBFetchNetroutesByNetnicIdAndNetvlanId($netnicid, $netvlanid, $inuse='t')
	{
		global $DB;
		if($netvlanid != null)
		{
			$dh = $DB->prepare("SELECT * FROM netroutes WHERE netnic_id=:netnicid AND netvlan_id=:netvlanid AND inuse=:inuse");
			$result = $dh->execute(array(":netnicid" => $netnicid,
									 ":netvlanid" => $netvlanid,
								 	  ":inuse" => $inuse));
									  
		}
		else
		{
			$dh = $DB->prepare("SELECT * FROM netroutes WHERE netnic_id=:netnicid AND netvlan_id is null AND inuse=:inuse");
			$result = $dh->execute(array(":netnicid" => $netnicid,
								 	  ":inuse" => $inuse));
			
		}
		
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
	
 	function DBFetchNetRoutesInUseAndByNetnicIdAndVlanId($netnicid, $netvlanid)
	{
		return  DBFetchNetroutesByNetnicIdAndNetvlanId($netnicid, $netvlanid, 't');
	}

	function DBFetchNetrouteByNetworkAndNetmask($network, $netmask)
	{
		global $DB;
		$dh = $DB->prepare("SELECT * FROM netroutes WHERE network=:network AND netmask=:netmask AND inuse='t'");
		$result = $dh->execute(array(":network" => $network,
									 ":netmask" => $netmask));
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
	
	function DBFetchNetrouteByNetworkAndNetmaskAndGateway($network, $netmask, $gateway)
	{
		global $DB;
		$dh = $DB->prepare("SELECT * FROM netroutes WHERE network=:network AND netmask=:netmask AND gateway=:gateway AND inuse='t'");
		$result = $dh->execute(array(":network" => $network,
									 ":netmask" => $netmask,
								 	 ":gateway" => $gateway));
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
	
	function DBAddNetroute($netnicid, $netvlanid, $network, $netmask, $gateway, $inuse, $description)
	{
		global $DB;
		$dh = $DB->prepare("INSERT INTO netroutes (netnic_id, netvlan_id, network, netmask, gateway, inuse, description) VALUES (:netnicid, :netvlanid, :network, :netmask, :gateway, :inuse, :description)");
		$result = $dh->execute(array(":netnicid" => $netnicid,
									 ":netvlanid" => $netvlanid,
								 	 ":network" => $network,
								 	":netmask" => $netmask,
									":gateway" => $gateway,
									":inuse" => $inuse,
									":description" => $description));
		if($result)
		{
			return $DB->lastInsertId("netroutes_id_seq");				
		}
		return 0;
	}
	
	function DBUpdateNetroute($id, $netnicid, $netvlanid, $network, $netmask, $gateway, $inuse, $description)
	{
		global $DB;
		$dh = $DB->prepare("UPDATE netroutes SET netnic_id=:netnicid, netvlan_id=:netvlanid, network=:network, netmask=:netmask, gateway=:gateway, inuse=:inuse, description=:description WHERE id=:id");
		$result = $dh->execute(array(	":id" => $id,
										":netnicid" => $netnicid,
										":netvlanid" => $netvlanid,
										":network" => $network,
										":netmask" => $netmask,
										":gateway" => $gateway,
										":inuse" => $inuse,
										":description" => $description));
		if($result)
		{
			return 1;
		}
		else
		{
			return 0;
		}
	}
	
	function DBDeleteNetrouteById($id)
	{
		global $DB;
		$dh = $DB->prepare("DELETE FROM netroutes WHERE id=:id");
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