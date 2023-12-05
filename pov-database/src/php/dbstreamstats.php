<?php
	require_once "dblib.php";
	
	function DBFetchStreamstatsByStreaminterfaceIdAndPathnumber($streaminterfaceid, $pathnumber = ""){
		global $DB;
		
		if($pathnumber == "" || $pathnumber == 100)
		{
			$sth = $DB->prepare("WITH t AS (SELECT *, timestamp::timestamp(0) AS truncated_timestamp FROM streamstats where streaminterface_id=:streaminterfaceid AND (pathnumber = 100 OR pathnumber IS NULL)) SELECT * FROM t  ORDER BY timestamp  DESC");
			$sth->execute(array(":streaminterfaceid" => $streaminterfaceid));
			
		}
		else
		{
			$sth = $DB->prepare("WITH t AS (SELECT *, timestamp::timestamp(0) AS truncated_timestamp FROM streamstats where streaminterface_id=:streaminterfaceid AND (pathnumber=:pathnumber)) SELECT * FROM t  ORDER BY timestamp  DESC");
			$sth->execute(array(":streaminterfaceid" => $streaminterfaceid,
								":pathnumber" => $pathnumber));			
		}

		$result = $sth->fetchAll();
		if($result === false){
			return array();
		}
		return $result;

	}
?>