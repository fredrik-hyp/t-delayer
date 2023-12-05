<?php
		require_once("dblib.php");
		
		class DBOrganisation
		{
			public $id = null;
			public $name = null;
			public $organisations = array();
			
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
					$this->${$key} = $element;
				}
			}
		
			function fromArray($arr)
			{
				if(!is_array($arr))
				{
					return false;
				}
			
				isset($arr['name']) ? $this->name = $arr['name'] : $this->name = "";
			
				return true;
			}
		
			function fetchAll()
			{
				$sth = $this->db->prepare("select * from organisations order by name");
				$sth->execute(array());
				$result = $sth->fetchAll();

				if($result === false)
				{
					return false;
				}
			
				$this->organisations = array();
				foreach($result as $org)
				{
					$classorg = new DBOrganisation($DB);
					$classorg->fromArray($org);
					$classorg->id = $result['id'];
					$this->organisations[] = $classorg;
				}
				return true;
				
			}
			
			function fetch($id)
			{
				if($id < 1)
				{
					return false;
				}
			
				$sth = $this->db->prepare("select * from organisations where id=:id");
				$sth->execute(array(":id" => $id));
				$result = $sth->fetch();
				if($result === false){
					return false;
				}
			
				$this->id = $result['id'];
				$this->name = $result['name'];
				return true;
			}
				
			function fetchByName($name)
			{
				$sth = $this->db->prepare("select * from organisations where name like :name");
				$sth->execute(array(":name" => $name));
				$result = $sth->fetch();
				if($result === false){
					return false;
				}
			
				$this->id = $result['id'];
				$this->name = $result['name'];
				return true;
				
			}
			
			function add()
			{
				if(trim($this->name) == "")
				{
					return false;
				}
				$testorg = new DBOrganisations($this->db);
				$testorg->fetchbyname(trim($this->name));
				if($testorg->id > 0)
				{
					return false;
				}
				
				$sth = $this->db->prepare("insert into organisations (name) values (:name)");
				$res = $sth->execute(array(":name" => $this->name));
				if($res === false)
				{
					return false;
				}
				
				$this->id = $this->db->lastInsertId("organisations_id_seq");
				return true;
			}
			
			function update()
			{
				if($this->id < 1)
				{
					return false;
				}
			
				if(trim($this->name) == "") 
				{
					return false;
				}
				
				$sth = $this->db->prepare("update organisations set name=:name where id=:id");
				$result = $sth->execute(array(
												":id" => $this->id,
												":name" => $this->name
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
			
				$sth = $this->db->prepare("delete from organisations where id=:id");
				$result = $sth->execute(array(":id" => $this->id));
			}
		}
			
		function DBFetchOrganisations()
        {
            global $DB;
            $sth = $DB->prepare("SELECT * FROM organisations ORDER BY name");
            $sth->execute(array());
            $result = $sth->fetchAll();
//			$organisations = DBFetch("organisations", "*", "", "name");
            if(!$result)
            {
                return array();
            }
			return $result;
		}
		
		function DBFetchOrganisationById($id)
        {
            global $DB;
			if(!is_numeric($id)){
				return array();
			}
            
            $sth = $DB->prepare("SELECT * from organisations WHERE id=:id");
            $sth->execute(array(":id" => $id));
            $result = $sth->fetch();
	//		$id = DBEscapeString($id);
//			$organisation = DBFetch("organisations", "*", "id=$id", "");
            if(!$result)
            {
                return array();
            }
			return $result;
		}
		
		function DBFetchOrganisationByName($name)
        {
			global $DB;
			$sth = $DB->prepare("select * from organisations where name ilike :name");
			$sth->execute(array(":name" => $name));
			$result = $sth->fetch();
			if($result === false)
            {
				return array();
			}
			return $result;
		}
		
		function DBFetchOrganisationByIdAndWithUserId($id, $userid)
        {
            global $DB;
			if(!is_numeric($id)){
				return array();
			}
			else if(!is_numeric($userid)){
				return array();
			}
			
			$id = DBEscapeString($id);
			$userid = DBEscapeString($userid);
            $sth = $DB->prepare("SELECT * from organisationanduser,  organisations WHERE organisationanduser.organisation_id=:id AND organisationanduser.user_id=:userid AND organisationanduser.organisation_id=organisations.id");
            $sth->execute(array(":id" => $id,
                                ":userid" => $id));
            $result = $sth->fetch();
            if($result === false)
            {
                return array();
            }
//			$organisation = DBFetch("organisationanduser, organisations", "organisations.*", "organisationanduser.organisation_id=$id AND organisationanduser.user_id=$userid AND organisationanduser.organisation_id=organisations.id", "");
			return $result;
		}
		
		function DBFetchOrganisationsWithUserId($userid)
        {
            global $DB;
			if(!is_numeric($userid))
            {
				return array();
			}
            
            
			$userid = DBEscapeString($userid);
            $sth = $DB->prepare("SELECT organisations.* FROM organisationanduser, organisations WHERE nisationanduser.organisation_id=organisations.id AND organisationanduser.user_id=:userid");
		//	$result = DBFetch("organisationanduser, organisations", "organisations.*", "organisationanduser.organisation_id=organisations.id AND organisationanduser.user_id=$userid");
            $sth->execute(array(":userid" => $userid));
            $result = $sth->fetchAll();
            if($result  === false)
            {
                return array();
            }
			return $result;
		}
		
		function DBFetchOrganisationWithDeviceId($deviceid)
        {
			global $DB;
			if(!is_numeric($deviceid))
            {
				return array();
			}
			
			$sth = $DB->prepare("SELECT organisations.* FROM organisations,organisationanddevice WHERE organisations.id=organisationanddevice.organisation_id AND organisationanddevice.device_id=:deviceid");
			$result = $sth->execute(array(":deviceid" => $deviceid));
			
			if($result === false)
            {
				return array();
			}
			return $sth->fetch();
		}
        
		function DBConnectOrganisationWithDevice($id, $deviceid)
        {
			global $DB;
			if(!is_numeric($id)){
				return -1;
			}
			if(!is_numeric($deviceid)){
				return -2;
			}
			
			$sth = $DB->prepare("INSERT INTO organisationanddevice (organisation_id, device_id) VALUES (:organisationid, :deviceid)");
			$result = $sth->execute(array(":organisationid" => $id, ":deviceid" => $deviceid) );
			if(!$result){
				return 0;
			}
			return 1;
		}
		
		function DBDisconnectOrganisationFromDevice($id, $deviceid)
        {
			global $DB;
			if(!is_numeric($id))
            {
				return -1;
			}
			if(!is_numeric($deviceid))
            {
				return -2;
			}
			
			$sth = $DB->prepare("DELETE FROM organisationanddevice WHERE organisation_id=:organisationid AND device_id=:deviceid");
			$result = $sth->execute(array(":organisationid" => $id, ":deviceid" => $deviceid));
			if(!$result){
				return 0;
			}
			return 1;
		}
		
		function DBUpdateOrganisation($id, $name, $address1="", $address2="", $zipcode="", $city="", $country="", $contactperson="", $telephone="", $email="", $comments="")
        {
			global $DB;
			$name = trim($name);
			if(!is_numeric($id))
            {
				return -1;
			}
			if($name == "")
            {
				return -2;
			}
			
			$sth = $DB->prepare("update organisations set name=:name, address1=:address1, address2=:address2, zipcode=:zipcode, city=:city, country=:country, contactperson=:contactperson, telephone=:telephone, email=:email, comments=:comments where id = :id");
			$result = $sth->execute(array(":id" => $id,
			 							  ":name" => $name,
										  ":address1" => $address1,
										  ":address2" => $address2,
										  ":zipcode" => $zipcode,
										  ":city" => $city,
										  ":country" => $country,
										  ":contactperson" => $contactperson,
										  ":telephone" => $telephone,
										  ":email" => $email,
										  ":comments" => $comments
									));
			if(!$result)
            {
				return 0;
			}
				return 1;
		}
		
		function DBAddOrganisation($name, $address1="", $address2="", $zipcode="", $city="", $country="", $contactperson="", $telephone="", $email="", $comments="")
        {
			global $DB;
			$name = trim($name);
			if($name == ""){
				return -1;
			}
			$sth = $DB->prepare("insert into organisations (name, address1, address2, zipcode, city, country, contactperson, telephone, email, comments) values (:name, :address1, :address2, :zipcode,:city, :country, :contactperson, :telephone, :email, :comments)");
			$result = $sth->execute(array(":name" => $name,
										  ":address1" => $address1,
										  ":address2" => $address2,
										  ":zipcode" => $zipcode,
										  ":city" => $city,
										  ":country" => $country,
										  ":contactperson" => $contactperson,
										  ":telephone" => $telephone,
										  ":email" => $email,
										  ":comments" => $comments
									));
			if($result)
            {
				return $DB->lastInsertId("organisations_id_seq");
			}
			
			return 0;

		}
		
		function DBDeleteOrganisation($id)
        {
			global $DB;
			if($id==""
            ){
				return -1;
			}
			$sth = $DB->prepare("delete from organisations where id=:id");
			$result = $sth->execute(array(":id" => $id));
			if($result)
            {
				return $sth->rowCount();
			}
			return -2;
		}
?>