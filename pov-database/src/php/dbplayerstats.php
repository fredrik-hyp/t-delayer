<?php
	require_once("dblib.php");
	
	function DBFetchPlayerstatsByStreaminterfaceId($streaminterfaceid)
    {
		global $DB;
		$sth = $DB->prepare("SELECT * FROM playerstats WHERE streaminterface_id=:streaminterfaceid ORDER BY pathnumber");
		$sth->execute(array(":streaminterfaceid" => $streaminterfaceid));
		$result = $sth->fetchAll();
		if($result === false)
        {
			return array();
		}
		return $result;
	}
?>