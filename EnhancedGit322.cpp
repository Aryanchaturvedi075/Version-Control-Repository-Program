#include <filesystem>
#include <iostream>
#include <fstream>
#include <sstream>
#include <memory>
#include <string>

using namespace std;
namespace fs = filesystem;
using directory = fs::directory_iterator;

// Class for each saved version of a file
class SavedVersion{
    public:
        string content;
        int version;
        shared_ptr<SavedVersion> next = nullptr;                            // Smart Pointers to ensure no Memory Leaks
        // Constructors
        SavedVersion(){}
        SavedVersion(string content, int versionNum): 
            content(content), version(versionNum){}

        // Overloading << Operator for printing saved versions of a file
        friend ostream& operator<<(ostream& os, const SavedVersion* cur){
            os << "\nVersion number: " << cur->version << endl;             // Print Version Information
            os << "Hash Value: " << hash<string>{}(cur->content) << endl;
            os << "Content: " << cur->content << endl;
            return os;
        }
};

// Helper Class to return multiple values from a single function
class Pointers{
    public:
        shared_ptr<SavedVersion> ptr1 = nullptr, ptr2 = nullptr;
        bool isNull;
        // Constructor used by traverse() function to return values to Git322::load() and Git322::remove()
        Pointers(shared_ptr<SavedVersion> ptr1, shared_ptr<SavedVersion> ptr2): 
            ptr1(ptr1), ptr2(ptr2){}
        // Constructor used by traverse() function to return values to Git322::compare()
        Pointers(shared_ptr<SavedVersion> ptr1, shared_ptr<SavedVersion> ptr2, bool emptyPtr): 
            ptr1(ptr1), ptr2(ptr2), isNull(emptyPtr){}
};

class LinkedList{
    // Private Attributes
    shared_ptr<SavedVersion> head = nullptr, tail = nullptr;
    int listSize = 0;

    public:
        // Getters
        shared_ptr<SavedVersion> getHead(){ return head; }
        int size(){ return listSize; }

        // Adding a new Node
        void add(shared_ptr<SavedVersion> file){
            if (listSize == 0) head = file;                                 // Case: Empty list
            else tail->next = file;
            tail = file;
            listSize++;
        }

        // Removing a Node
        void remove(shared_ptr<SavedVersion> prevFile){
            auto fileRmv = prevFile->next;
            prevFile->next = fileRmv->next;
            fileRmv->next = nullptr;
            if (head == fileRmv) head = prevFile->next;                     // Case: Deleting Head node
            if (tail == fileRmv) tail = prevFile;                           // Case: Deleting Tail node
            listSize--;
        }

        // Comparing contents of 2 Nodes, used in Git322::add() and Git322::compare()
        bool hash_file_diff(string oldVersion, string newVersion){
            size_t h1 = hash<string>{}(oldVersion);
            size_t h2 = hash<string>{}(newVersion);
            return h1 == h2;
        }

        // Traversing the LinkedList, used in Git322::load() and Git322::remove()
        shared_ptr<Pointers> traverse(int version){
            // Prev pointer used specifically to remove a node, i.e. Don't care when loading a node
            // Instantiated blank node to re-direct next pointer when deleting and reassigning head node
            shared_ptr<SavedVersion> prev(new SavedVersion);
            auto cur = head;
            prev->next = cur;
            // Traverse linkedList to identify node and its predecessor
            while(cur && cur->version != version){
                prev = cur;                                                 // Deallocates the blank node if not deleting the head node
                cur = (cur->next);
            }
            shared_ptr<Pointers> p(new Pointers(cur, prev));                // Return both pointers as members of a Pointer object
            return p;
        }

        // Overloading the traverse() function to use in Git322::compare()
        shared_ptr<Pointers> traverse(int version1, int version2){
            shared_ptr<SavedVersion> ver1 = nullptr, ver2 = nullptr;
            auto cur = head;
            while (cur){                                                    // identifying both nodes in linkedList
                if (cur->version == version1) ver1 = cur;
                if (cur->version == version2) ver2 = cur;
                if (ver1 != NULL && ver2 != NULL) break;
                cur = cur->next;
            }
            // If both versions were found, loop ends before reaching end of linkedList
            shared_ptr<Pointers> p(new Pointers(ver1, ver2, !cur));         // isNull = (cur == NULL)
            return p;
        }
        
        // Overloading the traverse() function to use in Git322::add() and Git322::search()
        bool traverse(string content, bool searching, bool noPrint){
            // Second argument indicates the caller of the function. Third argument used in Git322::search() function
            auto cur = head;
            while(cur){
                if (searching){
                    // Called from Git322::search() funtion
                    int position = cur->content.find(content);              // find keyword in current node
                    if (position >= 0) {
                        if (noPrint) return true;
                        else cout << cur;
                    }
                }
                else{
                    // Called from Git322::add() function
                    bool identical = hash_file_diff(cur->content, content); // compare hash values of both strings
                    if (identical) return true;
                }
                cur = cur->next;
            }
            return false;
        }
};

class Git322{
    // Private Members
    string fileName;
    int numVersions = 0;
    friend class EnhancedGit322;                                            // Friend class declaration
    string instructions = "To add the content of your file to version control press \t'a'\n"
                          "To remove a version press \t\t\t\t\t'r'\n"
                          "To load a version press \t\t\t\t\t'l'\n"
                          "To print to the screen the detailed list of all versions press \t'p'\n"
                          "To compare any 2 versions press \t\t\t\t'c'\n"
                          "To search versions for a keyword press \t\t\t\t's'\n"
                          "To exit press \t\t\t\t\t\t\t'e'\n";
    
    // Helper methods
    void list_mismatched_lines(string s1, string s2){
        // Ensure that both strings end with a newline character
        if (s1.substr(s1.size()-1, s1.size()) != "\n") s1.append("\n");
        if (s2.substr(s2.size()-1, s2.size()) != "\n") s2.append("\n");
        int index1 = s1.find("\n"), index2 = s2.find("\n"), i = 1;          // Iterate based on index of "/n" in each line
        cout << endl;
        while (index1 >= 0 || index2 >= 0){
            string line1 = "<Empty Line>", line2 = "<Empty Line>";          // Obtain topmost lines of both strings
            if(index1 >= 0) line1 = s1.substr(0, index1);
            if(index2 >= 0) line2 = s2.substr(0, index2);
            bool identical = mylist.hash_file_diff(line1, line2);           // Comparison of the 2 lines
            if (identical) cout << "Line " << i << ": " << "<Identical>" << endl;
            else cout << "Line " << i << ": " << line1 << " <<>> " << line2 << endl;
            s1 = s1.substr(index1+1), s2 = s2.substr(index2+1);             // Remove the compared lines from the original strings
            index1 = s1.find("\n"), index2 = s2.find("\n");
            i++;
        }
    }
    string read_from_file(string file){                                     // read contents of the file into a string
        ifstream input(file);
        if (!input.is_open()){
            cerr << "Failed to read the contents of the given file name" << endl;
            return "";                                                      // return blank string if unable to read file
        }
        auto ss = ostringstream{};                                          // convert inputstream to string
        ss << input.rdbuf();
        input.close();
        string text = ss.str();
        return text;
    }
    bool write_to_file(string file, string content, int version){           // write content into a given file
            ofstream output(file);
            if (!output.is_open()){
                cerr << "Failed to save version " << version << "'s content in a file" << endl;
                return true;                                                // true implies failed to write content into file
            }
            output << content;
            output.close();
            return false;
    }

    protected:
        LinkedList mylist;

    public:
        // Constructors
        Git322(){ cout << instructions; }
        Git322(string fileName): fileName(fileName){ cout << instructions; }
        // Destructor
        virtual ~Git322(){ cout << "Git322 versioning system has terminated." << endl; }

        virtual void add(string content){
            // Traverse mylist in case node already exists
            bool verExists = mylist.traverse(content, false, false);
            if (verExists){                                                 // Case: a node with the same content already exists
                cerr << "git322 did not detect any change to your file and will not create a new version."<< endl;
                return;
            }
            // Instantiate new node and add to linkedList
            shared_ptr<SavedVersion> file(new SavedVersion(content, ++numVersions));
            mylist.add(file);
            cout << "Your content has been added successfully." << endl;
        }

        void print(){
            cout << "Number of versions: " << mylist.size() << endl;
            auto cur = mylist.getHead();                                    // Print all nodes in mylist
            while (cur){
                cout << cur;
                cur = cur->next;
            }
        }

        void load (int version){
            // Traverse mylist to identify node
            auto p = mylist.traverse(version);                              // p = shared_ptr<Pointers>
            auto cur = p->ptr1;
            if (!cur){                                                      // Case: given node was not found
                cerr << "Please enter a valid version number.\n";
                cerr << "If you are not sure please press 'p' to list all valid version numbers." << endl;
                return;
            }
            // Write node contents into file
            bool failed = write_to_file(fileName, cur->content, cur->version);
            if (failed) return;
            cout << "Version " << cur->version << " loaded successfully.";
            cout << "Please refresh your text editor to see the changes." << endl;
        }

        void compare(int version1, int version2){
            auto p = mylist.traverse(version1, version2);
            // If cur reached end of mylist, then a file wasn't found
            bool notFound = p->isNull;
            if (notFound){                                                  // Case: one of the given versions was not found
                cerr << "One of the provided version numbers was invalid. Please enter a valid version number.\n";
                cerr << "If you are not sure please press 'p' to list all valid version numbers." << endl;
                return;
            }
            string s1 = p->ptr1->content, s2 = p->ptr2->content;            // Compare contents of both strings
            list_mismatched_lines(s1, s2);
        }

        void search(string keyword){                                        // Identify any node containing keyword
            bool containsKeyword = mylist.traverse(keyword, true, true);
            if (!containsKeyword){                                          // Case: none of the nodes contain the given keyword
                cout << "Your keyword '" << keyword << "' was not found in any version." << endl;
                return;
            }
            cout << "The keyword '" << keyword << "' has been found in the following versions:" << endl;
            mylist.traverse(keyword, true, false);
        }

        virtual void remove(int version){                                   // Traverse mylist to identify node and its predecessor
            auto p = mylist.traverse(version);
            auto cur = p->ptr1, prev = p->ptr2;
            if (!cur){                                                      // Case: given node was not found
                cerr << "Please enter a valid version number.\n";
                cerr << "If you are not sure please press 'p' to list all valid version numbers." << endl;
                return;
            }
            mylist.remove(prev);                                            // Remove prev->next node from linkedlist
            cout << "Version " << version << " deleted successfully."<< endl;
        }

        void execute(){
            char command;
            cout << "\nPick action: ";
            cin >> command;
            while(command != 'e'){                                          // Program runs while 'e' is not passed as command
                if (command == 'p') print();
                else if (command == 'i') cout << instructions;
                else if (command == 'a'){
                    string text = read_from_file(fileName);
                    if (!text.empty()) add(text);
                }
                else if (command == 'r'){
                    int version;
                    cout << "Enter the number of the version that you want to delete: ";
                    cin >> version;
                    remove(version);
                }
                else if (command == 'l'){
                    int version;
                    cout << "Which version would you like to load? ";
                    cin >> version;
                    load(version);
                }
                else if (command == 's'){
                    string keyword;
                    cout << "Please enter the keyword that you are looking for: ";
                    cin >> keyword;
                    search(keyword);
                }
                else if (command == 'c'){
                    int version1, version2;
                    cout << "Please enter the number of the first version to compare: ";
                    cin >> version1;
                    cout << "Please enter the number of the second version to compare: ";
                    cin >> version2;
                    compare(version1, version2);
                }
                else cout << "Unrecognized command. Press 'i' for instructions" << endl;
                cout << "\nChoose next action: ";
                cin >> command;
            }
        }
};

class EnhancedGit322: public Git322{
    // Helper Private methods
    // Used by EnhancedGit322::add() and EnhancedGit322::remove() methods
    string format_file_name(int version){                                   // Forms strings corresponding to the file and version
        int index = fileName.find(".");                                     // e.g. "file_3.txt"
        string file = fileName.substr(0, index);
        string extension = fileName.substr(index);
        string fileVersion = file+"_"+to_string(version)+extension;
        return fileVersion;
    }
    // Used by constructor to filter through all files in current directory
    int find_file_path(string path){                                        // Find starting index of the file from the given path
        int ext = fileName.find(".");                                       // e.g. "file_" in "c:/folders/projects/file_3.txt"
        string searchName = fileName.substr(0, ext);
        searchName.append("_");                                             
        int index = path.find(searchName);
        return index;
    }

    // Used by constructor to add existing file versions directly to mylist
    void add_file_to_list(string path, int index){                          // Obtain file name from filtered results e.g. "file_3.txt"
        string node = path.substr(index);
        int ext = node.find(".");
        char number = node[ext-1];
        int version = number - '0';
        string text = read_from_file(node);                                 // Read file content and add as new node to mylist
        if (text.empty()) return;
        shared_ptr<SavedVersion> file(new SavedVersion(text, version));
        if (version > numVersions) numVersions = version;
        mylist.add(file);
    }

    public:
        // Constructor
        EnhancedGit322(string fileName){
            this->fileName = fileName;
            // filter to find files that are saved versions
            for (const auto& file : directory(fs::current_path())){         // Used directory_iterator here
                string path = file.path().string();
                int index = find_file_path(path);
                // Add nodes to mylist
                if (index >= 0) add_file_to_list(path, index);
            }
        }
        // Overriding Git::add()
        virtual void add(string content) override{                          // Add file externally as saved version
            Git322::add(content);
            string fileVersion = format_file_name(numVersions);
            write_to_file(fileVersion, content, numVersions);
        }

        // Overriding Git322::remove()
        virtual void remove(int version) override{                          // Delete file from external folder
            Git322::remove(version);
            string fileVersion = format_file_name(version);
            try { fs::remove(fileVersion); }
            catch(const fs::filesystem_error& err) { cerr << "filesystem error: " << err.what() << endl;}
        }
};

int main(){
    cout << "Welcome to the Comp322 file versioning system!" << endl;
    string fileName = "file.txt";

    ifstream input (fileName);                                              // Ensure file is readable
    if (!input.is_open()) {
        cerr << "Failed to read the contents of the given file name" << endl;
        return 1;
    }
    input.close();

    EnhancedGit322 repository = EnhancedGit322(fileName);
    repository.execute();                                                   // Main function running the interactive loop
    return 0;
}
