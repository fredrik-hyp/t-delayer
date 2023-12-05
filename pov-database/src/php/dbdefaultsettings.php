<?php

	require_once("dblib.php");
	
	function DBFetchDefaultsettings(){
		global $DB;
		$sth = $DB->prepare("SELECT * FROM defaultsettings");
		$result = $sth->execute();
		if($result){
			return $sth->fetch();
		}
		
		return array();
	}
	
	function DBUpdateDefaultsettings($id, $imstunnelconnecthost, $imstunnelconnectport){
		global $DB;
		if(!is_numeric($id)){
			return -1;
		}
		
		$sth = $DB->prepare("UPDATE defaultsettings SET imstunnelconnecthost=:imstunnelconnecthost, imstunnelconnectport=:imstunnelconnectport WHERE id=:id");
		$result = $sth->execute(array(":imstunnelconnecthost" => $imstunnelconnecthost,
							 "imstunnelconnectport" => $imstunnelconnectport,
							 ":id" => $id));
		if(!$result){
			return 0;
		}
		return 1;
	}
?>