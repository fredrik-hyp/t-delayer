<?php
	require_once("dblib.php");
	
	function DBFetchAllVirtualSources()
    {
		global $DB;
		$sth = $DB->prepare("select * from virtualsources order by name");
		$sth->execute(array());
		$result = $sth->fetchAll();
		if($result === false)
        {
			return array();
		}
		return $result;
	}
    
	function DBFetchVirtualSourceById($id)
	{
		global $DB;
		
		
		$sth = $DB->prepare("select * from virtualsources where id=:id");
		$sth->execute(array(":id" => $id));
		$result = $sth->fetch();
		if($result === false)
		{
			return array();
		}
		
		return $result;
	}
	
     function DBFetchVirtualsourcesByVirtualSourceGroupId($virtualsourcegroupid)
     {
         global $DB;
         $sth = $DB->prepare("select * from virtualsources where virtualsourcegroup_id=:virtualsourcegroupid order by name");
         $sth->execute(array(":virtualsourcegroupid" => $virtualsourcegroupid));
         $result = $sth->fetchAll();
         if($result === false)
         {
             return array();
         }
         
         return $result;
     }
     
     function DBFetchAllVirtualSourcesWhithoutGroups()
     {
         global $DB;
         $sth = $DB->prepare("select * from virtualsources where virtualsourcegroup_id is null order by name");
         $sth->execute(array());
         $result = $sth->fetchAll();
         if($result === false)
         {
             return array();
         }
         
         return $result;
     }
?>