<?php
	require_once("dblib.php");
	
	function DBFetchAllVirtualSourceGroups()
    {
		global $DB;
		$sth = $DB->prepare("select * from virtualsourcegroups order by name");
		$sth->execute(array());
		$result = $sth->fetchAll();
		if($result === false)
        {
			return array();
		}
		return $result;
	}
    
    function DBFetchVirtualSourceGroupRootParents()
    {
        global $DB;
        $sth = $DB->prepare("select * from virtualsourcegroups where virtualsourcegroup_id is null order by name");
        $sth->execute(array());
        $result = $sth->fetchAll();
        if($result === false)
        {
            return array();
        }
        
        return $result;
    }
    
    function DBFetchVirtualSourceGroupsByParentId($virtualsourcegroupid)
    {
        global $DB;
        $sth = $DB->prepare("select * from virtualsourcegroups where virtualsourcegroup_id = :virtualsourcegroupid order by name");
        $sth->execute(array(":virtualsourcegroupid" => $virtualsourcegroupid));
        $result = $sth->fetchAll();
        if($result === false)
        {
            return array();
        }
        
        return $result;
    }
?>