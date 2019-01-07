import collections
import os
import xml.etree.ElementTree as xmltree
import yaml

from . import console
from . import fspath



class AwPluginTree(object):
    
    def __init__(self):
        super(AwPluginTree, self).__init__()
        self.__plugins = {}

        filelist = fspath.listfiles(fspath.plugins(), relative=True)
        nodelist = list()
        for filepath in filelist:
            fkey, fext = os.path.splitext(filepath)
            if fext in [".yaml", ".xml"]:
                if fkey not in self.__plugins:
                    self.__plugins[fkey] = AwPluginNode(self, fkey)
            else:
                console.warning("plugin ignored: unknown extension ({})".format(filepath))

        for plugin in self.__plugins.values():
            plugin.load(fspath.plugins())

    def find(self, path):
        return self.__plugins[path]

    def scan(self, path):
        return filter(lambda node: node.startswith(path), self.__plugins.keys())

    def dump(self):
        for name in sorted(self.__plugins.keys()):
            print "=================================================="
            self.__plugins[name].dump()



class AwPluginNode(object):

    def __init__(self, tree, path):
        self.__tree  = tree
        self.__path  = path
        self.__args  = collections.OrderedDict()
        self.__rule  = collections.OrderedDict()
        self.__panel = None
        self.__frame = None

    def todict(self):
        data = {}
        data["type"]  = "Node" if self.isnode() else "Leaf"
        data["args"]  = self.__args
        data["rule"]  = self.__rule.values()
        data["panel"] = self.__panel
        data["frame"] = self.__frame
        return data

    def isnode(self):
        return bool(self.__rule)

    def isleaf(self):
        return not self.isnode()

    def tree(self):
        return self.__tree

    def path(self):
        return self.__path

    def load(self, rootpath):
        filepath = os.path.join(rootpath, self.path())

        # load xml (roslaunch)
        xml_args = []
        if os.path.exists(filepath + ".xml"):
            xroot = xmltree.parse(filepath + ".xml").getroot()
            for xnode in xroot:
                if ("arg" == xnode.tag) and ("value" not in xnode.attrib):
                    xml_args.append(xnode.attrib)

        # load yaml
        if os.path.exists(filepath + ".yaml"):
            with open(filepath+ ".yaml") as fp:
                ydata = yaml.safe_load(fp)

            self.__panel = ydata.get("panel", {"view": "default.panel"})
            self.__frame = ydata.get("frame", {"view": "default.frame"})

            #self.__info
            self.__args  = ydata.get("args",  {})
            for data in ydata.get("rules", []): self.__rule[data["name"]] = data

        console.warning(filepath)
        # validation
        args_type = ["str", "int", "real", "tf", "file", "filelist", "calib"]
        rule_type = ["unit", "list"]
        for data in self.__args:
            if data["type"] not in args_type: raise Exception("Plugin Args Type: " + filepath)
        for name, data in self.__rule.items():
            if data["type"] not in rule_type: raise TypeError("Plugin Rule Type: " + filepath)

    def panel(self):
        return self.__panel

    def frame(self):
        return self.__frame

    def args(self):
        return self.__args

    def rules(self):
        return self.__rule.values()

    def error(self, text):
        console.error("{}: {} ({})".format(self.__class__.__name__, text, self.__nodepath))

    def optional_children(self):
        plugins = {}
        for rule in self.rules():
            if rule["type"] == "optional":
                plugins[rule["name"]] = self.__tree.scan(rule["plugin"])
        return plugins

    def default_config(self):
        config = {}
        #for argkey, argdef in self.__args:
        #    cfgkey = "args." + argkey
        #    config[cfgkey] = argdef.get("default", "")
        return config

    # temporary
    def argstr(self, config):
        lines = []
        for argkey, argdef in self.__args.items():
            cfgkey = "args." + argkey
            lines.append(argkey + ": " + config[cfgkey])
        return "\n".join(lines)


if __name__ == "__main__":
    plugin = AwPluginTree()
    plugin.dump()