<?php
	require_once "dblib.php";
	
	function DBFetchStreaminterfaceRunInfoByStreaminterfaceId($streaminterfaceid){
		global $DB;
		$sth = $DB->prepare("select * from streaminterfaceruninfos where streaminterface_id=:streaminterfaceid");
		$sth->execute(array(":streaminterfaceid" => $streaminterfaceid));
		$result = $sth->fetchAll();
		if($result === false){
			return array();
		}
		return $result;
	}
?>