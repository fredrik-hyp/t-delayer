<?php
	require_once("dblib.php");
	
	function DBFetchLastNLinesStreaminterfacelogsByStreaminterfaceId($streaminterfaceid, $nroflines){
		global $DB;
		if(!is_numeric($nroflines)){
			return array();
		}
		
		$sth = $DB->prepare("WITH t AS (SELECT *, timestamp::timestamp(0) AS truncated_timestamp FROM streaminterfacelogs where streaminterface_id=:streaminterfaceid ORDER BY timestamp  DESC LIMIT :nroflines) SELECT * FROM t ORDER BY timestamp ASC");
		$sth->execute(array(":streaminterfaceid" => $streaminterfaceid, ":nroflines" => $nroflines));
		$result = $sth->fetchAll();
		if($result === false){
			return array();
		}
		return $result;
	}
    
	function DBFetchStreaminterfacelogsByStreaminterfaceId($streaminterfaceid, $sortorder="ASC"){
		global $DB;
		
        if($sortorder != "ASC" && $sortorder != "DESC"){
            return false;
        }
        
		$sth = $DB->prepare("SELECT * FROM streaminterfacelogs WHERE streaminterface_id=:streaminterfaceid ORDER BY timestamp $sortorder");
		$sth->execute(array(":streaminterfaceid" => $streaminterfaceid));
		$result = $sth->fetchAll();
		if($result === false){
			return array();
		}
		return $result;
	}
    
?>