<?php
	require_once("dblib.php");
function DBFetchNetServicesByType($type)
	{
		global $DB;
		$dh = $DB->prepare("SELECT * FROM netservices WHERE type=:type");
		
		
		$result = $dh->execute(array(":type" => $type));
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

	function DBUpdateNetService($id, $type, $address, $options)
	{
		global $DB;
		$dh = $DB->prepare("UPDATE netservices SET type=:type, address=:address, options=:options WHERE id=:id");
		
		$result = $dh->execute(array(":id" => $id,
									 ":type" => $type,
								 	 ":address" => $address,
								 	 ":options" => $options));

		if($result === false)
		{
			return 0;						 	
		}
		
		return 1;
	}

	function DBAddNetService($type, $address, $options)
	{
		global $DB;
		$dh = $DB->prepare("INSERT INTO netservices (type, address, options) VALUES (:type, :address, :options)");

		$result = $dh->execute(array(":type" => $type, ":address" => $address, ":options" => $options));

		if($result)
		{
			//return $DB->lastInsertId("netservices_id_seq");
			return 1;				
		}
		return 0;
	}
	
	function DBDeleteNetService($id)
	{
		global $DB;
		$dh = $DB->prepare("DELETE FROM netservices WHERE id=:id");
		$result = $dh->execute(array("id" => $id));
		
		if($result === false)	
		{
			return false;
		}
		return true;
	}
?>

