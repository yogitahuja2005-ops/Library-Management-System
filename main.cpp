#include <algorithm>
#include <chrono>
#include <cctype>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <regex>
#include <sstream>
#include <string>
#include <vector>
using namespace std;
// Single-file, dependency-free library application. Data is written as standard JSON.
// -------------------- DATA STRUCTURES --------------------
// Book stores one title and its copy counts.
struct Book {
    int id, total, available, borrows;
    string title, author, category;
};
// Member stores the details of a registered library user.
struct Member {
    int id;
    string name, department;
    bool active;
};
// Loan stores one issue/return transaction.
struct Loan {
    int id, member, book;
    string issued, due, returned, status;
};
// Verification stores the librarian's physical return check.
struct Verification {
    int loan, member, book;
    string returned, status, verified;
};
// -------------------- LIBRARY CLASS --------------------
// This class owns the records and implements the library rules.
class Library {
    vector<Book> books;
    vector<Member> members;
    vector<Loan> loans;
    vector<Verification> checks;
    int nextBook= 101, nextMember= 1001, nextLoan= 1;
    const string dataFile= "library_data.json";
    const string R= "\033[0m", G= "\033[32m", E= "\033[31m", Y= "\033[33m", C= "\033[36m", B= "\033[1m";
    // Convert text to lowercase so searches are case-insensitive.
    static string lower(string s) {
        transform(s.begin(), s.end(), s.begin(), [](unsigned char c){
            return char(tolower(c));
        }
        );
        return s;
    }
    static bool useful(const string& s) {
        return s.find_first_not_of(" \t\r\n") != string::npos;
    }
    static bool safeText(const string& s) {
        return useful(s) && s.find_first_of("\"\\\n\r") == string::npos;
    }
    // Return today's date; offset=14 gives the due date.
    static string date(int offset= 0) {
        time_t t= chrono::system_clock::to_time_t(chrono::system_clock::now()+ chrono::hours(24* offset));
        tm d{};
#ifdef _WIN32
        localtime_s(& d, & t);
#else
        localtime_r(& t, & d);
#endif
        ostringstream o;
        o<<put_time(& d, "%Y-%m-%d");
        return o.str();
    }
    static time_t parseDate(const string& s) {
        tm d{};
        istringstream i(s);
        i>> get_time(& d, "%Y-%m-%d");
        d.tm_hour= 12;
        return i.fail()?0:mktime(& d);
    }
    static int lateDays(const string& due) {
        time_t n= parseDate(date()), d= parseDate(due);
        return n> d ? int((n- d)/ 86400) : 0;
    }
    // Small helpers used to read the application's JSON file.
    static string arrayPart(const string& all, const string& name) {
        string marker= "\""+ name+ "\":[";
        size_t a= all.find(marker);
        if(a== string::npos) return "";
        a+= marker.size();
        int depth= 1;
        size_t p= a;
        for (;p<all.size()&& depth;p++){
            if(all[p]== '[')++ depth;
            else if(all[p]== ']')-- depth;
        }
        return depth?"":all.substr(a, p- a- 1);
    }
    static string property(const string& object, const string& key) {
        regex r("\\\""+ key+ "\\\":\\\"([^\\\"]*)\\\"");
        smatch m;
        return regex_search(object, m, r)?m[1].str():"";
    }
    static int number(const string& object, const string& key) {
        regex r("\\\""+ key+ "\\\":(-?[0-9]+)");
        smatch m;
        return regex_search(object, m, r)?stoi(m[1].str()):0;
    }
    static bool boolean(const string& object, const string& key) {
        return object.find("\""+ key+ "\":true")!= string::npos;
    }
    static vector<string> objects(const string& part) {
        vector<string> out;
        regex r("\\{([^{}]*)\\}");
        for (sregex_iterator i(part.begin(), part.end(), r), e;i!= e;++ i) out.push_back((* i)[1].str());
        return out;
    }
    // Find records by ID. A nullptr means that the ID does not exist.
    Book* getBook(int id) {
        for (auto& x:books) if(x.id== id)return & x;
        return nullptr;
    }
    Member* getMember(int id) {
        for (auto& x:members) if(x.id== id)return & x;
        return nullptr;
    }
    Loan* getLoan(int id) {
        for (auto& x:loans) if(x.id== id)return & x;
        return nullptr;
    }
    const Book* getBookC(int id) const {
        for (const auto& x:books) if(x.id== id)return & x;
        return nullptr;
    }
    const Member* getMemberC(int id) const {
        for (const auto& x:members) if(x.id== id)return & x;
        return nullptr;
    }
    int activeLoans(int member) const {
        int n= 0;
        for (const auto& x:loans)if(x.member== member&& x.status== "ISSUED")++ n;
        return n;
    }
    // -------------------- TERMINAL DISPLAY HELPERS --------------------
    void line(char c= '-') const {
        cout<<string(93, c)<<'\n';
    }
    void heading(const string& s) const {
        cout<<"\n"<<B<<C<<" "<<s<<R<<'\n';
        line('=');
    }
    void good(const string& s) const {
        cout<<G<<"[SUCCESS] "<<s<<R<<'\n';
    }
    void bad(const string& s) const {
        cout<<E<<"[ERROR] "<<s<<R<<'\n';
    }
    void info(const string& s) const {
        cout<<Y<<"[INFO] "<<s<<R<<'\n';
    }
    void pause() const {
        cout<<"\nPress Enter to continue...";
        string s;
        getline(cin, s);
    }
    // Keep asking until the user enters a valid whole number.
    int askNumber(const string& label, bool zero= false) const {
        while (true){
            cout<<label;
            string s;
            getline(cin, s);
            stringstream ss(s);
            int n;
            char x;
            if(ss>> n&& ! (ss>> x)&& (zero?n>= 0:n> 0))return n;
            bad("Please enter a valid positive whole number.");
        }
    }
    string askText(const string& label) const {
        while (true){
            cout<<label;
            string s;
            getline(cin, s);
            if(safeText(s))return s;
            bad("Enter text without quotes, backslashes, or blank content.");
        }
    }
    void booksTable(const vector<Book> & a) const {
        cout<<left<<setw(6)<<"ID"<<setw(30)<<"TITLE"<<setw(22)<<"AUTHOR"<<setw(16)<<"CATEGORY"<<right<<setw(7)<<"TOTAL"<<setw(8)<<"FREE"<<setw(8)<<"LOANS"<<'\n';
        line();
        for (const auto& x:a)cout<<left<<setw(6)<<x.id<<setw(30)<<x.title.substr(0, 29)<<setw(22)<<x.author.substr(0, 21)<<setw(16)<<x.category.substr(0, 15)<<right<<setw(7)<<x.total<<setw(8)<<x.available<<setw(8)<<x.borrows<<'\n';
    }
public:
    void load() {
        ifstream f(dataFile);
        if (!f)return;
        string s((istreambuf_iterator<char> (f)), {});
        try {
            for (const auto& o:objects(arrayPart(s, "books"))) books.push_back({
                number(o, "id"), number(o, "total"), number(o, "available"), number(o, "borrows"), property(o, "title"), property(o, "author"), property(o, "category")
            }
            );
            for (const auto& o:objects(arrayPart(s, "members"))) members.push_back({
                number(o, "id"), property(o, "name"), property(o, "department"), boolean(o, "active")
            }
            );
            for (const auto& o:objects(arrayPart(s, "loans"))) loans.push_back({
                number(o, "id"), number(o, "member"), number(o, "book"), property(o, "issued"), property(o, "due"), property(o, "returned"), property(o, "status")
            }
            );
            for (const auto& o:objects(arrayPart(s, "verifications"))) checks.push_back({
                number(o, "loan"), number(o, "member"), number(o, "book"), property(o, "returned"), property(o, "status"), property(o, "verified")
            }
            );
            for (const auto& x:books) nextBook= max(nextBook, x.id+ 1);
            for (const auto& x:members) nextMember= max(nextMember, x.id+ 1);
            for (const auto& x:loans) nextLoan= max(nextLoan, x.id+ 1);
        } catch(...) {
            books.clear();
            members.clear();
            loans.clear();
            checks.clear();
            bad("Data file could not be read; an empty library was loaded.");
        }
    }
    // Save all four record collections to library_data.json.
    bool save() const {
        ofstream f(dataFile);
        if (!f){
            bad("Unable to save library_data.json.");
            return false;
        }
        auto q= [](const string& s){
            return "\""+ s+ "\"";
        };
        f<<"{\n\"books\":[";
        for (size_t i= 0;i<books.size();++ i){
            const auto& x= books[i];
            f<<"{\"id\":"<<x.id<<",\"title\":"<<q(x.title)<<",\"author\":"<<q(x.author)<<",\"category\":"<<q(x.category)<<",\"total\":"<<x.total<<",\"available\":"<<x.available<<",\"borrows\":"<<x.borrows<<"}"<<(i+ 1<books.size()?",":"");
        }
        f<<"],\n\"members\":[";
        for (size_t i= 0;i<members.size();++ i){
            const auto& x= members[i];
            f<<"{\"id\":"<<x.id<<",\"name\":"<<q(x.name)<<",\"department\":"<<q(x.department)<<",\"active\":"<<(x.active?"true":"false")<<"}"<<(i+ 1<members.size()?",":"");
        }
        f<<"],\n\"loans\":[";
        for (size_t i= 0;i<loans.size();++ i){
            const auto& x= loans[i];
            f<<"{\"id\":"<<x.id<<",\"member\":"<<x.member<<",\"book\":"<<x.book<<",\"issued\":"<<q(x.issued)<<",\"due\":"<<q(x.due)<<",\"returned\":"<<q(x.returned)<<",\"status\":"<<q(x.status)<<"}"<<(i+ 1<loans.size()?",":"");
        }
        f<<"],\n\"verifications\":[";
        for (size_t i= 0;i<checks.size();++ i){
            const auto& x= checks[i];
            f<<"{\"loan\":"<<x.loan<<",\"member\":"<<x.member<<",\"book\":"<<x.book<<",\"returned\":"<<q(x.returned)<<",\"status\":"<<q(x.status)<<",\"verified\":"<<q(x.verified)<<"}"<<(i+ 1<checks.size()?",":"");
        }
        f<<"]\n}\n";
        return true;
    }
    // -------------------- FEATURES / MENU OPERATIONS --------------------
    void dashboard() const {
        heading("LIBRARY MANAGEMENT SYSTEM | Dashboard");
        int issued= 0, pending= 0, over= 0;
        for (const auto& x:loans)if(x.status== "ISSUED"){
            ++ issued;
            if(lateDays(x.due)> 0)++ over;
        }
        for (const auto& x:checks)if(x.status== "PENDING")++ pending;
        cout<<" Titles: "<<books.size()<<" Members: "<<members.size()<<" Active loans: "<<issued<<"\n Awaiting verification: "<<pending<<" Overdue: "<<over<<'\n';
        line();
    }
    void addBook(){
        heading("Add Book");
        string t= askText("Title: "), a= askText("Author: "), c= askText("Category: ");
        int n= askNumber("Copies: ");
        books.push_back({
            nextBook++, n, n, 0, t, a, c
        }
        );
        save();
        good("Book added.");
    }
    void listBooks()const{
        heading("Book Catalogue");
        if(books.empty())info("No books registered.");
        else booksTable(books);
    }
    void search(){
        heading("Search Books");
        string q= lower(askText("Title, author, or category: "));
        vector<Book> out;
        for (const auto& x:books)if(lower(x.title).find(q)!= string::npos|| lower(x.author).find(q)!= string::npos|| lower(x.category).find(q)!= string::npos)out.push_back(x);
        if(out.empty())info("No matching books.");
        else booksTable(out);
    }
    void addMember(){
        heading("Register Member");
        members.push_back({
            nextMember++, askText("Name: "), askText("Department: "), true
        }
        );
        save();
        good("Member registered.");
    }
    void listMembers()const{
        heading("Members");
        if(members.empty()){
            info("No members registered.");
            return;
        }
        cout<<left<<setw(8)<<"ID"<<setw(32)<<"NAME"<<setw(28)<<"DEPARTMENT"<<"ACTIVE LOANS\n";
        line();
        for (const auto& x:members)cout<<left<<setw(8)<<x.id<<setw(32)<<x.name.substr(0, 31)<<setw(28)<<x.department.substr(0, 27)<<activeLoans(x.id)<<'\n';
    }
    // Issue a book after checking member status, loan limit and stock.
    void issue(){
        heading("Issue Book | 14 days, maximum 3 active loans");
        int m= askNumber("Member ID: "), b= askNumber("Book ID: ");
        Member* me= getMember(m);
        Book* bo= getBook(b);
        if (!me){
            bad("Member not found.");
            return;
        }
        if (!bo){
            bad("Book not found.");
            return;
        }
        if (!me-> active){
            bad("Member is inactive.");
            return;
        }
        if(activeLoans(m)>= 3){
            bad("This member has reached the 3-book limit.");
            return;
        }
        if(bo-> available<1){
            bad("No verified available copy exists.");
            return;
        }
        -- bo-> available;
        ++ bo-> borrows;
        loans.push_back({
            nextLoan++, m, b, date(), date(14), "", "ISSUED"
        }
        );
        save();
        good("Issued successfully. Due: "+ loans.back().due);
    }
    // A return becomes pending; stock is not restored yet.
    void returnBook(){
        heading("Return Book | sends copy to physical verification");
        int id= askNumber("Loan ID: ");
        Loan* l= getLoan(id);
        if (!l){
            bad("Loan not found.");
            return;
        }
        if(l-> status!= "ISSUED"){
            bad("Only an active issued loan may be returned.");
            return;
        }
        l-> returned= date();
        l-> status= "RETURNED_PENDING";
        checks.push_back({
            l-> id, l-> member, l-> book, l-> returned, "PENDING", ""
        }
        );
        save();
        good("Return recorded. Available stock is unchanged until verification.");
    }
    // Librarian marks a pending return as FOUND or MISSING.
    void verify(){
        heading("Physical Book Verification");
        bool any= false;
        cout<<left<<setw(8)<<"LOAN"<<setw(9)<<"BOOK"<<setw(11)<<"MEMBER"<<setw(16)<<"RETURNED"<<"STATUS\n";
        line();
        for (const auto& x:checks)if(x.status== "PENDING"){
            any= true;
            cout<<left<<setw(8)<<x.loan<<setw(9)<<x.book<<setw(11)<<x.member<<setw(16)<<x.returned<<x.status<<'\n';
        }
        if (!any){
            info("No returns are pending.");
            return;
        }
        int id= askNumber("Loan ID to verify: ");
        Verification* v= nullptr;
        for (auto& x:checks)if(x.loan== id&& x.status== "PENDING")v= & x;
        if (!v){
            bad("That loan has no pending verification.");
            return;
        }
        cout<<"Result (F = found, M = missing): ";
        string r;
        getline(cin, r);
        if(r.empty()|| (toupper(r[0])!= 'F'&& toupper(r[0])!= 'M')){
            bad("Use F or M.");
            return;
        }
        Loan* l= getLoan(id);
        if (!l){
            bad("Associated loan was not found.");
            return;
        }
        v-> verified= date();
        if(toupper(r[0])== 'F'){
            v-> status= "FOUND";
            l-> status= "FOUND";
            Book* b= getBook(v-> book);
            if(b&& b-> available<b-> total)++ b-> available;
            good("Found: availability increased by one.");
        } else{
            v-> status= "MISSING";
            l-> status= "MISSING";
            good("Missing: stock was not increased.");
        }
        save();
    }
    // Display active loans whose due dates have passed.
    void overdue()const{
        heading("Automatic Overdue Detection");
        bool any= false;
        cout<<left<<setw(7)<<"LOAN"<<setw(25)<<"BOOK"<<setw(25)<<"MEMBER"<<setw(14)<<"DUE DATE"<<"DAYS LATE\n";
        line();
        for (const auto& x:loans)if(x.status== "ISSUED"&& lateDays(x.due)> 0){
            any= true;
            auto* b= getBookC(x.book);
            auto* m= getMemberC(x.member);
            cout<<left<<setw(7)<<x.id<<setw(25)<<(b?b-> title.substr(0, 24):"Unknown")<<setw(25)<<(m?m-> name.substr(0, 24):"Unknown")<<setw(14)<<x.due<<lateDays(x.due)<<'\n';
        }
        if (!any)info("No overdue books today.");
    }
    // Rank books by past borrowing count and suggest restocking.
    void demand()const{
        heading("Smart Demand Analysis");
        vector<Book> r= books;
        sort(r.begin(), r.end(), [](const Book& a, const Book& b){
            return a.borrows!= b.borrows?a.borrows> b.borrows:a.title<b.title;
        }
        );
        if(r.empty()){
            info("No borrowing data yet.");
            return;
        }
        cout<<left<<setw(5)<<"#"<<setw(30)<<"TITLE"<<right<<setw(10)<<"BORROWS"<<setw(10)<<"FREE"<<setw(14)<<"SUGGESTION"<<'\n';
        line();
        int p= 1;
        for (const auto& x:r){
            string s= (x.borrows>= 3&& x.available<= 1)?"Add 1 copy":"-";
            if(x.borrows>= 5&& x.available<= 2)s= "Add 2 copies";
            cout<<left<<setw(5)<<p++ <<setw(30)<<x.title.substr(0, 29)<<right<<setw(10)<<x.borrows<<setw(10)<<x.available<<setw(14)<<s<<'\n';
        }
        info("Rule: popular books (3+ loans) with low verified availability are recommended.");
    }
    void history()const{
        heading("Borrowing History");
        if(loans.empty()){
            info("No loan history.");
            return;
        }
        cout<<left<<setw(7)<<"LOAN"<<setw(24)<<"BOOK"<<setw(24)<<"MEMBER"<<setw(12)<<"ISSUED"<<setw(12)<<"DUE"<<"STATUS\n";
        line();
        for (const auto& x:loans){
            auto* b= getBookC(x.book);
            auto* m= getMemberC(x.member);
            cout<<left<<setw(7)<<x.id<<setw(24)<<(b?b-> title.substr(0, 23):"Unknown")<<setw(24)<<(m?m-> name.substr(0, 23):"Unknown")<<setw(12)<<x.issued<<setw(12)<<x.due<<x.status<<'\n';
        }
    }
    void stats()const{
        heading("Library Statistics");
        int total= 0, free= 0, issued= 0, pending= 0, missing= 0;
        for (const auto& x:books){
            total+= x.total;
            free+= x.available;
        }
        for (const auto& x:loans)if(x.status== "ISSUED")++ issued;
        for (const auto& x:checks){
            if(x.status== "PENDING")++ pending;
            if(x.status== "MISSING")++ missing;
        }
        cout<<" Titles: "<<books.size()<<"\n Registered physical copies: "<<total<<"\n Verified available copies: "<<free<<"\n Members: "<<members.size()<<"\n Active loans: "<<issued<<"\n Returns awaiting verification: "<<pending<<"\n Missing copies recorded: "<<missing<<'\n';
    }
    // Main menu loop: load once, handle choices, save on exit.
    void run(){
        load();
        while (true){
            cout<<"\033[2J\033[H";
            dashboard();
            cout<<" 1. Book management (add, list, search)\n 2. Member management (add, list)\n 3. Issue book\n 4. Return book\n 5. Physical book verification\n 6. Overdue books\n 7. Smart demand analysis\n 8. Borrowing history\n 9. Library statistics\n 0. Exit\n";
            line();
            int c= askNumber("Choose an option: ", true);
            if(c== 0){
                save();
                cout<<G<<"Data saved. Goodbye.\n"<<R;
                break;
            }
            if(c== 1){
                cout<<"\n1. Add book\n2. List books\n3. Search books\n";
                int x= askNumber("Choose: ");
                if(x== 1)addBook();
                else if(x== 2)listBooks();
                else if(x== 3)search();
                else bad("Invalid option.");
            } else if(c== 2){
                cout<<"\n1. Register member\n2. List members\n";
                int x= askNumber("Choose: ");
                if(x== 1)addMember();
                else if(x== 2)listMembers();
                else bad("Invalid option.");
            } else if(c== 3)issue();
            else if(c== 4)returnBook();
            else if(c== 5)verify();
            else if(c== 6)overdue();
            else if(c== 7)demand();
            else if(c== 8)history();
            else if(c== 9)stats();
            else bad("Invalid menu option.");
            pause();
        }
    }
};
// Program entry point.
int main(){
    ios::sync_with_stdio(false);
    Library app;
    app.run();
}
