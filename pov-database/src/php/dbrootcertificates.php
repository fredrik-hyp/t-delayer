<?php
    require_once("dblib.php");
    
    
    function DBFetchRootCertificateById($id)
    {
        global $DB;

		$sth = $DB->prepare("select * from rootcertificates where id=:id");
		$sth->execute(array(":id" => $id));
		$result = $sth->fetch();
		if($result === false)
        {
			return array();
		}
		return $result;
		
    }


    function DBFetchRootCertificateByOrganisationId($organisationid){
        global $DB;

		$sth = $DB->prepare("select * from rootcertificates where organisation_id=:organisationid");
		$sth->execute(array(":organisationid" => $organisationid));
		$result = $sth->fetch();
		if($result === false)
		{
			return array();
		}
		return $result;
		
    }
    
    function DBUpdateRootCertificate($id, $created, $expiration_days, $organisationid, $name, $public_part, $private_part, $notbefore, $notafter){
        global $DB;

		$organisationid = trim($organisationid);
		if($organisationid == "")
		{
			return -1;
		}
        
        if(trim($created) == "")
		{
            $created = "now()";
        }
		
		$sth = $DB->prepare("update rootcertificates set created=:created, expiration_days=:expiration_days, organisation_id=:organisationid, name=:name, public_part=:public_part, private_part=:private_part, notbefore=:notbefore, notafter=:notafter where id=:id");
		$result = $sth->execute(array(":created" => $created,
									  ":expiration_days" => $expiration_days,
									  ":organisationid" => $organisationid,
									  ":name" => $name,
									  ":public_part" => $public_part,
									  ":private_part" => $private_part,
									  ":notbefore" => $notbefore,
									  ":notafter" => $notafter,
                                      ":id" => $id
								));
        if(!$result)
		{
            return 0;
        }
        return 1;
    }

    function DBAddRootCertificate($created, $expiration_days, $organisationid, $name, $public_part, $private_part, $notbefore, $notafter)
    {
        global $DB;

		$organisationid = trim($organisationid);
		if($organisationid == "")
        {
			return -1;
		}
        
        if(trim($created) == "")
        {
            $created = "now()";
        }
        
		$sth = $DB->prepare("insert into rootcertificates (created, expiration_days, organisation_id, name, public_part, private_part, notbefore, notafter) values (:created, :expiration_days, :organisationid, :name,:public_part, :private_part, :notbefore, :notafter)");
		$result = $sth->execute(array(":created" => $created,
									  ":expiration_days" => $expiration_days,
									  ":organisationid" => $organisationid,
									  ":name" => $name,
									  ":public_part" => $public_part,
									  ":private_part" => $private_part,
									  ":notbefore" => $notbefore,
									  ":notafter" => $notafter
								));
		if($result !== false)
        {
			return $DB->lastInsertId("rootcertificates_id_seq");
		}
        return $result;
    }
    
    function DBDeleteRootCertificateById($id){
        global $DB;
        if(trim($id) == "")
		{
            return -1;
        }
        
        $sth = $DB->prepare("delete from rootcertificates where id=:id");
        $sth->execute(array(":id" => $id));
		if($result)
		{
			return $sth->rowCount();
		}
        return -2;
    }
?>