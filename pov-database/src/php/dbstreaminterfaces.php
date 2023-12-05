<?php

	require_once("dblib.php");

	class DBStreaminterface
	{
		public $id = null;
		public $name = null;
		public $usertext = null;
		public $db = null;
		
		function __construct($db, $id = null)
		{
			$this->db = $db;
			if($id != null)
			{
				$this->fetch($id);
			}
		}
		
		function fromArray($arr)
		{
			if(!is_array($arr))
			{
				return false;
			}
			
			
			isset($arr['usertext']) ? $this->usertext = $arr['usertext'] : $this->usertext = "";
			
			return true;
		}
		
		function fetch($id)
		{
			if($id < 1)
			{
				return false;
			}
			
			$sth = $this->db->prepare("select * from streaminterfaces where id=:id");
			$sth->execute(array(":id" => $id));
			$result = $sth->fetch();
			if($result === false)
			{
				 error_log ("Could not fetch streaminterface with id $id.");
				return false;
			}
			
			$this->id = $result['id'];
			$this->name = $result['name'];
			$this->usertext = $result['usertext'];
			return true;
		}
		
		function fetchAll()
		{
			$sth = $this->db->prepare("select * from streaminterfaces order by id");
			$sth->execute(array());
			$result = $sth->fetchAll();

			if($result === false)
			{
				return false;
			}
		
			$this->streaminterfaces = array();
			foreach($result as $stream)
			{
				$classstream = new DBStreaminterfaces($DB);
				$classstream->fromArray($stream);
				$classstream->id = $result['id'];
				$this->streaminterfaces[] = $classstream;
			}
			return true;		
		}
			
		function update()
		{
			if($this->id < 1)
			{
				return false;
			}
			
			$sth = $this->db->prepare("update streaminterfaces set name=:name, usertext=:usertext where id=:id");
			$result = $sth->execute(array(
											":id" => $this->id,
											":name" => $this->name,
											":usertext" => $this->usertext
										));

			if(!$result)
			{
				return false;
			}
			return true;
		}
		
		function delete()
		{
			if($this->id  < 1)
			{
				return false;
			}
			
			$sth = $this->db->prepare("delete from streaminterfaces where id=:id");
			$result = $sth->execute(array(":id" => $this->id));
		}
	}
	
	function DBFetchAllStreaminterfaces(){
		global $DB;
		$sth = $DB->prepare("select * from streaminterfaces order by id");

		$result = $sth->execute();
		if($result){
			return $sth->fetchAll();
		}
		return array();
	}
	
	function DBFetchStreaminterfaceById($id){
		global $DB;
		$sth = $DB->prepare("select * from streaminterfaces where id=:id");
		$sth->execute(array(":id" => $id));
		$result = $sth->fetch();
		if($result === false){
			return array();
		}
		return $result;
	}
	
	function DBFetchStreaminterfaceByName($name){
		global $DB;
		$sth = $DB->prepare("select * from streaminterfaces where name ilike :name");
		$sth->execute(array(":name" => $name));
		$result = $sth->fetch();
		if($result === false){
			return array();
		}
		return $result;
	}
	
	function DBUpdateStreaminterface($id, $name, $usertext){
		global $DB;
		$sth = $DB->prepare("update streaminterfaces set name=:name, usertext=:usertext where id=:id");
		$result = $sth->execute(array(
										":id" => $id,
										":name" => $name,
										":usertext" => $usertext));

		if(!$result){
			return 0;
		}
			return 1;
	}
	

?>