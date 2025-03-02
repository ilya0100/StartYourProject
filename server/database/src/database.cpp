#include "database.hpp"


mysqlx::Table MainDataBase::CreateTable(const mysqlx::string &name, const mysqlx::string &params) {
    std::cout<<"Creating table"<<std::endl;
    mysqlx::string quoted_name = mysqlx::string("`")
    + sqlconn->getDefaultSchemaName()
    + mysqlx::string(L"`.`") + name + mysqlx::string(L"`");
    mysqlx::string create = "CREATE TABLE IF NOT EXISTS ";
    create += quoted_name;
    create += params;
    sqlconn->sql(create).execute();
    return sqlconn->getDefaultSchema().getTable(name);
}

void MainDataBase::CreateDatabase(std::string db_name) {
    cli = std::make_unique<mysqlx::Client>( "root:123qwerty@localhost:33060/", mysqlx::ClientOption::POOL_MAX_SIZE, 7);
    sqlconn = std::make_unique<mysqlx::Session>(cli->getSession());
    sqlconn->sql( "CREATE DATABASE IF NOT EXISTS " + db_name + ";").execute();
}

void MainDataBase::connect() {
    CreateDatabase(SYP_DB_NAME);

    cli = std::make_unique<mysqlx::Client>( "root:123qwerty@localhost:33060/"SYP_DB_NAME, mysqlx::ClientOption::POOL_MAX_SIZE, 7);
    sqlconn = std::make_unique<mysqlx::Session>(cli->getSession());

    user_data_table = std::make_unique<mysqlx::Table>(
        CreateTable(USER_TABLE, "( user_name  CHAR(32) NOT NULL, email  CHAR(32),  name  CHAR(32),"
                " sur_name  CHAR(32), user_description  CHAR(255), password  CHAR(64) NOT NULL, PRIMARY KEY ( user_name ))"));
                
    project_data_table = std::make_unique<mysqlx::Table>(
        CreateTable(PROJECT_TABLE,"( user_name  char(32) NOT NULL, project_name  char(32), "
        " team_name  char(32),  project_description  char(255),  diversity  double, primary key( project_name ),"
        " foreign key ( user_name ) references user_data( user_name ) ON DELETE CASCADE ON UPDATE CASCADE)" ));
        
    token_data_table = std::make_unique<mysqlx::Table>(CreateTable(TOKEN_TABLE,
        "( token char(100), user_name  CHAR(32) NOT NULL unique,primary key (token),"
        " FOREIGN KEY (user_name)  REFERENCES user_data (user_name) ON DELETE CASCADE ON UPDATE CASCADE)"));
        
    notification_data_table = std::make_unique<mysqlx::Table>(CreateTable(NOTIFICATION_TABLE,  
        "(id  int NOT NULL AUTO_INCREMENT, project_name  char(32) NOT NULL, motivation_words"
        "  char(255),user_name char(32) not null,status int, primary key( id ), "
        "foreign key ( project_name ) references project_data(project_name ) ON DELETE CASCADE ON UPDATE CASCADE,"
        " foreign key ( user_name ) references user_data( user_name ) ON DELETE CASCADE ON UPDATE CASCADE)"));
        
    team_data_table = std::make_unique<mysqlx::Table>(CreateTable(TEAM_TABLE,
        "(user_name char(32) not null, project_name char(32) not null,"
        "foreign key ( user_name ) references user_data( user_name)  ON DELETE CASCADE ON UPDATE CASCADE,"
        " foreign key ( project_name ) references project_data( project_name ) ON DELETE CASCADE ON UPDATE CASCADE,"
        " unique(user_name,project_name) )"));
        
    tags_data_table = std::make_unique<mysqlx::Table>(CreateTable(TAGS_TABLE, 
        "(id int auto_increment, tag char(30) unique,PRIMARY KEY ( id ) )"));

    user_tags_data_table = std::make_unique<mysqlx::Table>(CreateTable(USER_TAGS_TABLE, 
        "(user_name char(32) not null, id_tag int not null, "
        "foreign key ( user_name ) references user_data( user_name ) ON DELETE CASCADE ON UPDATE CASCADE, "
        "foreign key ( id_tag ) references tags_data( id ) ON DELETE CASCADE ON UPDATE CASCADE, unique(user_name,id_tag))"));
        
    project_tags_data_table = std::make_unique<mysqlx::Table>(CreateTable(PROJECT_TAGS_TABLE, 
        "(project_name char(32) not null, id_tag int not null,"
        " foreign key ( project_name ) references project_data( project_name ) ON DELETE CASCADE ON UPDATE CASCADE,"
        "foreign key ( id_tag ) references tags_data( id ) ON DELETE CASCADE ON UPDATE CASCADE, "
        "unique(project_name,id_tag))"));

    std::list<mysqlx::Schema> schemaList = sqlconn->getSchemas();

    std::cout << "Available schemas in this session:" << std::endl;


    // Loop over all available schemas and print their name
    for (mysqlx::Schema schema : schemaList) {
        std::cout << schema.getName() << std::endl;
    }
}




MainDataBase::~MainDataBase() {
    sqlconn->sql("DROP TABLE IF EXISTS "TOKEN_TABLE).execute();
    sqlconn->close();
}



DBStatus MainDataBase::DeleteFromPersonTable(std::string &username) {
    std::cout << "DeleteFromPersonTable:" << std::endl;

    user_data_table->remove()
    .where("user_name=:param")
    .bind("param",username)
    .execute();
    return DBStatus::ok;
}
DBStatus MainDataBase::DeleteToken(std::string& username) {
    std::cout << "DeleteToken:" << std::endl;

    token_data_table->remove()
    .where("user_name=:param")
    .bind("param",username)
    .execute();
    return DBStatus::ok;
}

DBStatus MainDataBase::DeleteFromPostTable(std::string& post) {
    std::cout << "DeleteFromPostTable:" << std::endl;
    project_data_table->remove()
    .where("project_name=:param")
    .bind("param",post)
    .execute();
    return DBStatus::ok;
}


//needs del
DBStatus MainDataBase::DeleteFromRequestToPostTable(RequestToPostData &data) {
    std::cout << "DeleteFromRequestToPostTable:" << std::endl;
    notification_data_table->remove()
    .where("project_name=:param1 and user_name=:param2")
    .bind("param1",data.project_name)
    .bind("param2", data.username)
    .execute();
    return DBStatus::ok;
    return DBStatus::ok;
}

Message<std::vector<RequestToPostData>, DBStatus> MainDataBase::FindRequestToPostTable(std::string &username) {
    std::cout << "FindRequestToPostTable:" << std::endl;

    return std::vector<RequestToPostData>();
}

Message<UserData, DBStatus> MainDataBase::FindIntoPersonByUsername(std::string &username) {
    std::cout << "FindIntoPersonByUsername:" << std::endl;
    mysqlx::RowResult res = user_data_table->select( "user_name", "email", "name", "sur_name", "user_description", "password")
    .where("user_name = :param")
    .orderBy("name")
    .bind("param",username)
    .execute();
    
    UserData data;
    mysqlx::Row row = res.fetchOne();
    
    if (row.isNull()) {
        return Message<UserData, DBStatus>(DBStatus::not_found);
    }

    data.username = mysqlx::string(row[0]);
    data.email = mysqlx::string(row[1]);
    data.name = mysqlx::string(row[2]);
    data.sur_name = mysqlx::string(row[3]);
    data.user_description = mysqlx::string(row[4]);
    data.password = mysqlx::string(row[5]);
    return Message<UserData, DBStatus>(data);
}


Message<std::vector<std::string>, DBStatus> MainDataBase::FindIntoTeambyProjectName(std::string &project_name){
    mysqlx::RowResult res = team_data_table->select("user_name", "project_name")
    .where("project_name= :param")
    .bind("param",project_name)
    .execute();
    mysqlx::Row row ;
    std::vector<std::string> teammates;
    while (row= res.fetchOne()){
        teammates.push_back(std::string(row[0]));
    }
    return Message<std::vector<std::string>, DBStatus>(teammates);
}

//выводит все проекты пользователя
Message<std::vector<std::string>, DBStatus> MainDataBase::SearchProjectNames(std::string &username){
    mysqlx::RowResult res = project_data_table->select("project_name")
    .where("user_name = :param")
    .bind("param", username)
    .execute();
    mysqlx::Row row;
    std::vector<std::string> projects;
    while (row = res.fetchOne()){
        std::string temp = std::string(row[0]);
        projects.push_back(temp);
    }
    return Message<std::vector<std::string>, DBStatus>(projects);
}

Message<ProjectData, DBStatus> MainDataBase::FindIntoPostTable(std::string &project_name) {
    mysqlx::RowResult res = project_data_table->select("user_name", "project_name", "team_name", "project_description", "diversity")
    .where("project_name = :param")
    .bind("param",project_name)
    .execute();

    ProjectData data;
    if(mysqlx::Row row = res.fetchOne()) {
        data.username = std::string(row[0]);
        data.project_name = std::string(row[1]);
        data.team_name = std::string(row[2]);
        data.project_description = std::string(row[3]);
        data.diversity = double(row[4]);
        auto msg = FindProjectsTags(data.project_name);
        data.post_tags = msg.data;
        msg = FindIntoTeambyProjectName(data.project_name);
        data.teammates = msg.data;
        return Message<ProjectData, DBStatus>(data);
    }
    return Message<ProjectData, DBStatus>(DBStatus::not_found);
}

//функция находит все записи, которые содержат определенный проджектнейм, далее пока есть записи ищет по id тега его значение
Message<std::vector<std::string>, DBStatus> MainDataBase::FindProjectsTags(std::string &project_name) {
    std::cout << "ProjectFindTag:" << std::endl;
    mysqlx::RowResult res = project_tags_data_table->select("project_name", "id_tag")
    .where("project_name = :param")
    .bind("param", project_name)
    .execute();
    mysqlx::Row row;
    std::vector<std::string> tags;
    while (row = res.fetchOne()) {
        int temp = row[1];
        auto msg = FindTagbyID(temp);
        tags.push_back(std::string(msg.data));
    }
    return Message<std::vector<std::string>, DBStatus>(tags);
}


Message<std::string, DBStatus> MainDataBase::FindTagbyID(int &id) {
    std::cout << "FindTag:" << std::endl;
    mysqlx::RowResult res = tags_data_table->select("id", "tag")
    .where("id = :param")
    .bind("param", id)
    .execute();
    mysqlx::Row row;
    if (!(row = res.fetchOne())) {
        return Message<std::string, DBStatus>(DBStatus::not_found);
    }
    return Message<std::string, DBStatus>(std::string(row[1]));
}


Message<int, DBStatus> MainDataBase::FindTagbyTagName(std::string &tag) {
    std::cout << "FindTag:" << std::endl;
    mysqlx::RowResult res = tags_data_table->select("id", "tag")
    .where("tag = :param")
    .bind("param", tag)
    .execute();
    mysqlx::Row row;
    if (!(row = res.fetchOne())) {
        return Message<int, DBStatus>(DBStatus::not_found);
    }
    return Message<int, DBStatus>(int(row[0]));
}

DBStatus MainDataBase::InsertIntoTeamTable(std::string &username, std::string &projectname){
    std::cout << "InsertIntoTeamsTable:" << std::endl;
    team_data_table->insert("user_name", "project_name")
    .values(username,projectname)
    .execute();
}



DBStatus MainDataBase::InsertIntoTagsTable(std::string &data){
     std::cout << "InsertIntoTagsTable:" << std::endl;
    auto msg = FindTagbyTagName(data);
    if (msg.status == DBStatus::not_found){    
        tags_data_table->insert("tag")
        .values(data)
        .execute();
    }   
    return DBStatus::ok;
}

DBStatus MainDataBase::InsertIntoProjectTagsTable(std::string &data, std::string &project_name){
    std::cout << "InsertIntoProjectTable:" << std::endl;
    if(InsertIntoTagsTable(data) == DBStatus::ok){
        auto msg = FindTagbyTagName(data);
        project_tags_data_table->insert( "project_name", "id_tag")
        .values(project_name, msg.data)
        .execute();
    }
    return DBStatus::ok;
}



DBStatus MainDataBase::InsertIntoPostTable(ProjectData &data) {
    std::cout << "InsertIntoPostTable:" << std::endl;
    project_data_table->insert("project_name", "team_name", "project_description", "diversity", "user_name")
    .values (data.project_name, data.team_name, data.project_description, data.diversity, data.username)
    .execute();

    for (int i = 0; i < data.post_tags.size() ; i++ ){
        InsertIntoProjectTagsTable(data.post_tags[i], data.project_name);
    }
    for (int i = 0; i < data.teammates.size() ; i++ ){
        InsertIntoTeamTable(data.teammates[i], data.project_name);
    }
    return DBStatus::ok;
}



DBStatus MainDataBase::InsertIntoUserTable(UserData &data) {
    std::cout << "InsertIntoUserTable:" << std::endl;

    user_data_table->insert("email", "user_name","password","name", "sur_name", "user_description")
    .values(data.email,data.username,data.password,data.name,data.sur_name,data.user_description).execute();

    return DBStatus::ok;
}



DBStatus MainDataBase::InsertIntoRequestToPostTable(RequestToPostData &data) {
    std::cout << "InsertIntoRequestToPostTable:" << std::endl;
    notification_data_table->insert("user_name", "post_id", "motivation_words", "status")
    .values(data.username,data.project_name,data.motivation_words, 3)
    .execute();
    return DBStatus::ok;
}


DBStatus MainDataBase::InsertToken(std::string &username, std::string& token) {
    std::cout << "InsertToken:" << std::endl;
    token_data_table->insert("user_name","token")
    .values(username, token)
    .execute();
    return DBStatus::ok;
}

//updatам дописать sql-запросы

DBStatus MainDataBase::EditUserInPersonTable(UserData &data) {
    std::cout << "EditUserInPersonTable:" << std::endl;
    user_data_table->update()
    .set("email", mysqlx::expr(":param2"))
    .set("name", mysqlx::expr(":param3"))
    .set("sur_name", mysqlx::expr(":param4"))
    .set("user_description", mysqlx::expr(":param5"))
    .set("password", mysqlx::expr(":param6"))
    .where("user_name=:param7")
    .bind("param2", data.email)
    .bind("param3", data.name)
    .bind("param4", data.sur_name)
    .bind("param5", data.user_description)
    .bind("param6", data.password)
    .bind("param7", data.username)
    .execute();

    return DBStatus::ok;
}

DBStatus MainDataBase::EditPostInPostTable(ProjectData &data) {
    std::cout << "EditPostInPostTable:" << std::endl;
    if (DeleteFromPostTable(data.project_name) != DBStatus::ok) {
        return DBStatus::unknown;
    }
    if (InsertIntoPostTable(data) != DBStatus::ok) {
        return DBStatus::unknown;
    }

    return DBStatus::ok;
}


DBStatus MainDataBase::EditRequestToPostTable(RequestToPostData &data){
    std::cout << "EditRequestToPostTable:" << std::endl;

    return DBStatus::ok;
}


Message<std::string, DBStatus> MainDataBase::FindToken(std::string &username) {
    std::cout << "FindToken:" << std::endl;
    mysqlx::RowResult res = token_data_table->select("user_name", "token")
    .where("user_name=:param")
    .bind("param", username)
    .execute();
    mysqlx::Row row;
    if (!(row = res.fetchOne())) {
        return Message<std::string, DBStatus>(DBStatus::not_found);
    }
    return Message<std::string, DBStatus>(std::string(row[1]));
}
