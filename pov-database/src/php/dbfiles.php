<?php

require_once("dblib.php");

class DBFile
{
	public $id;
	public $streaminterface_id;
	public $filename;
	public $size;
	public $priority;
	public $queuedtime;
	public $startsendtime;
	public $stopsendtime;
	public $transferredbytes;
	public $status;
	public $comment;
	public $files = array();
	
	function __construct($db, $id = null)
	{
		$this->db = $db;
		if($id != null)
		{
			$this->fetch($id);
		}
	}
	
	protected function copyRow($row)
	{
		
		foreach($row as $key => $element)
		{
			if(is_numeric($key)) continue;
			$this->{$key} = $element;
		}

	}
	
	function fromArray($arr)
	{
		if(!is_array($arr))
		{
			return false;
		}
		$this->id = $arr['id'];
		isset($arr['streaminterface_id']) ? $this->streaminterface_id = $arr['streaminterface_id'] : $this->streamerinterface_id = -1;
		isset($arr['filename']) ? $this->filename = $arr['filename']: $this->filename = "";
		isset($arr['size']) ? $this->size = $arr['size']: $this->size = 0;
		isset($arr['priority']) ? $this->priority = $arr['priority']: $this->priority = 0;
		isset($arr['queuedtime']) ? $this->queuedtime = $arr['queuedtime']: $this->queuedtime =  "";
		isset($arr['startsendtime']) ? $this->startsendtime = $arr['startsendtime']: $this->startsendtime = "";
		isset($arr['stopsendtime']) ? $this->stopsendtime = $arr['stopsendtime']: $this->stopsendtime = "";
		isset($arr['transferredbytes']) ? $this->transferredbytes = $arr['transferredbytes']: $this->transferredbytes = 0;
		isset($arr['status']) ? $this->status = $arr['status']: $this->status = "";
		isset($arr['comment']) ? $this->comment = $arr['comment'] : $this->comment = "";
		
		
		return true;
	}
	
	protected function rowsFromArray($rows)
	{
		$this->file = array();
		foreach($rows as $row)
		{
			$file = new DBFile($this->db);
			$file->fromArray($row);
			$this->files[] = $file;
		}
	}
	
	function toArray()
	{
		$result = array();
		$result['id'] = $this->id;
		$result['streaminterface_id'] = $this->streaminterface_id;
		$result['filename'] = $this->filename;
		$result['size'] = $this->size;
		$result['priority'] = $this->priority;
		$result['queuedtime'] = $this->queuedtime;
		$result['startsendtime'] = $this->startsendtime;
		$result['stopsendtime'] = $this->stopsendtime;
		$result['transferredbytes'] = $this->transferredbytes;
		$result['status'] = $this->status;
		$result['comment'] = $this->comment;
		
		return $result;
	}
	
	function filesToArray()
	{
	  $result = array();
	  foreach($this->files as $file)
	 {
           $result[] = $file->toArray();
	 }
	  return $result;
	}

	function fetch($id)
	{
		if($id < 1)
		{
			return false;
		}
		
		$sth = $this->db->prepare("SELECT * FROM files WHERE id=:id");
		$sth->execute(array(":id" => $id));
		$row = $sth->fetch();
		if($row === false)
		{
			error_log("could not fetch file with id $id.");
			return false;			
		}
		
		$this->copyRow($row);
		return true;
	}
	
	function fetchAll()
	{

		$sth = $this->db->prepare("select * from files order by queuedtime");
		$sth->execute(array());
		$rows = $sth->fetchAll();

		if($rows === false)
		{
			error_log("Could not fetch files from database.");
			return false;
		}
		$this->rowsFromArray($rows);
		return true;
	}
	
	function fetchHighestPriority()
	{	
		$sth = $this->db->prepare("select * from files where status != 'Done' and status !='OnHold' order by priority desc limit 1");
		$sth->execute(array());
		$row = $sth->fetch();
		if($row === false)
		{
			error_log("could not fetch file with highest priority $id.");
			return false;			
		}
		
		$this->copyRow($row);
		return true;
	}
	

	function fetchByStreaminterfaceId($streaminterfaceid)
	{
		if($streaminterfaceid < 1)
		{
			return false;
		}
		
		$sth = $this->db->prepare("select * from files where streaminterface_id=:streaminterfaceid order by id desc");
		$sth->execute(array(":streaminterfaceid" => $streaminterfaceid));

		$rows = $sth->fetchAll();
		if($rows === false)
		{
			error_log("Could not fetch files-.");
			return false;
		}
	
		$this->rowsFromArray($rows);
		return true;
		
	}
	
	function insert()
	{
		$sth = $this->db->prepare("insert into files (streaminterface_id, filename, priority, queuedtime, startsendtime, stopsendtime, transferredbytes, status, comment)
									values (:streaminterfaceid, :filename, :priority, :queuedtime, :startsendtime, :stopsendtime, :transferredbytes, :status, :comment)");
													
		$result = $sth->execute(array(":streaminterfaceid" => $this->streaminterface_id,
							":filename" => $this->filename,
							":priority" => $this->priority,
							":queuedtime" => $this->queuedtime,
							":startsendtime" => $this->startsendtime,
							":stopsendtime" => $this->stopsendtime,
							":transferredbytes" => $this->transferredbytes,
							":status" => $status,
							":comment" => $comment
		));
		
		if($result)
		{
			$this->id = $this->db->llastInsertId("files_id_seq");
			return true;
		}
		
		return false;
	}
	
	function update()
	{
		$sth = $this->db->prepare("update files set streaminterface_id=:streaminterfaceid,
													filename=:filename,
													priority=:priority,
													queuedtime=:queduedtime,
													startsendtime=:startsendtime,
													stopsendtime=:stopsendtime,
													transferredbytes=:transferredbytes,
													status=:status,
													commment=:comment where id=:id");
													
		$result = $sth->execute(array(":streaminterfaceid" => $this->streaminterface_id,
							":filename" => $this->filename,
							":priority" => $this->priority,
							":queuedtime" => $this->queuedtime,
							":startsendtime" => $this->startsendtime,
							":stopsendtime" => $this->stopsendtime,
							":transferredbytes" => $this->transferredbytes,
							":status" => $status,
							":comment" => $comment
		));
		
		return $result;
	}
	function updatePriority($filepriority)
	{
		$sth = $this->db->prepare("update files set priority=:priority where id=:id");
													
		$result = $sth->execute(array(":priority" => $filepriority, ":id" => $this->id));
		
		return $result;
	}

	function changeStatus($filestatus)
	{
		$sth = $this->db->prepare("update files set status=:status where id=:id");
		$result = $sth->execute(array(":status" => $filestatus, ":id" => $this->id));
		
		return $result;
	}

	function delete()
	{
		$sth = $this->db->prepare("delete from files where id=:id");
		$result = $sth->execute(array(":id" => $this->id));
		return $result;
	}
}

?>
